#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
using namespace std;

struct TaskNode
{
    int       id;
    string    title;
    bool      done;
    int       priority;    
    bool      inPlanner;   
    string    createdAt;
    TaskNode* next;

    TaskNode() : id(0), done(false), priority(5),
                 inPlanner(false), next(nullptr) {}
};

struct StackNode
{
    TaskNode*  task;
    StackNode* next;
};

class UndoStack
{
    StackNode* top;
public:
    UndoStack() : top(nullptr) {}

    void push(TaskNode* t)
    {
        TaskNode* copy = new TaskNode();
        *copy      = *t;
        copy->next = nullptr;

        StackNode* sn = new StackNode();
        sn->task = copy;
        sn->next = top;
        top      = sn;
    }

    TaskNode* pop()
    {
        if (!top) return nullptr;
        StackNode* tmp  = top;
        TaskNode*  task = tmp->task;
        top = top->next;
        delete tmp;
        return task;
    }

    bool isEmpty() const { return top == nullptr; }

    ~UndoStack()
    {
        while (!isEmpty()) { TaskNode* t = pop(); delete t; }
    }
};

struct HeapEntry
{
    int priority;
    int taskId;
};

class MinHeap
{
    static const int MAX = 500;
    HeapEntry heap[MAX];
    int       sz;

    int parent(int i) { return (i - 1) / 2; }
    int left(int i)   { return 2 * i + 1;   }
    int right(int i)  { return 2 * i + 2;   }

    void swp(int i, int j)
    {
        HeapEntry t = heap[i]; heap[i] = heap[j]; heap[j] = t;
    }

    void heapifyUp(int i)
    {
        while (i > 0 && heap[i].priority < heap[parent(i)].priority)
        { swp(i, parent(i)); i = parent(i); }
    }

    void heapifyDown(int i)
    {
        int s = i, l = left(i), r = right(i);
        if (l < sz && heap[l].priority < heap[s].priority) s = l;
        if (r < sz && heap[r].priority < heap[s].priority) s = r;
        if (s != i) { swp(i, s); heapifyDown(s); }
    }

public:
    MinHeap() : sz(0) {}

    void insert(int priority, int taskId)
    {
        if (sz >= MAX) { cout << "  [!] Heap full.\n"; return; }
        heap[sz] = {priority, taskId};
        heapifyUp(sz++);
    }

    HeapEntry peekMin() const { return heap[0]; }

    void removeById(int taskId)
    {
        int idx = -1;
        for (int i = 0; i < sz; i++)
            if (heap[i].taskId == taskId) { idx = i; break; }
        if (idx == -1) return;
        heap[idx] = heap[--sz];
        if (idx < sz) { heapifyUp(idx); heapifyDown(idx); }
    }

    void updatePriority(int taskId, int newPri)
    {
        for (int i = 0; i < sz; i++)
            if (heap[i].taskId == taskId)
            {
                heap[i].priority = newPri;
                heapifyUp(i); heapifyDown(i);
                return;
            }
    }

    void sortedSnapshot(HeapEntry* out, int& outSz) const
    {
        outSz = sz;
        for (int i = 0; i < sz; i++) out[i] = heap[i];
        for (int i = 0; i < outSz - 1; i++)
        {
            int m = i;
            for (int j = i + 1; j < outSz; j++)
                if (out[j].priority < out[m].priority) m = j;
            HeapEntry t = out[i]; out[i] = out[m]; out[m] = t;
        }
    }

    bool isEmpty() const { return sz == 0; }
    int  size()    const { return sz; }
};

class PlannerQueue
{
    static const int MAX = 100;
    int  data[MAX];
    int  front, rear, count;

public:
    PlannerQueue() : front(0), rear(0), count(0) {}

    bool enqueue(int taskId)
    {
        if (count >= MAX) return false;
        data[rear] = taskId;
        rear       = (rear + 1) % MAX;
        count++;
        return true;
    }

    int dequeue()
    {
        if (count == 0) return -1;
        int val = data[front];
        front   = (front + 1) % MAX;
        count--;
        return val;
    }

    int  peek()    const { return count > 0 ? data[front] : -1; }
    bool isEmpty() const { return count == 0; }
    int  size()    const { return count; }

    bool contains(int taskId) const
    {
        for (int i = 0; i < count; i++)
            if (data[(front + i) % MAX] == taskId) return true;
        return false;
    }

    void remove(int taskId)
    {
        int tmp[MAX], n = 0;
        for (int i = 0; i < count; i++)
        {
            int id = data[(front + i) % MAX];
            if (id != taskId) tmp[n++] = id;
        }
        front = 0; rear = n; count = n;
        for (int i = 0; i < n; i++) data[i] = tmp[i];
    }

    void getAll(int* out, int& outCount) const
    {
        outCount = count;
        for (int i = 0; i < count; i++)
            out[i] = data[(front + i) % MAX];
    }
};

class TodoList
{
    TaskNode*    head;
    int          idCounter;
    UndoStack    undoStack;
    MinHeap      heap;
    PlannerQueue planner;

    string currentTime()
    {
        time_t now = time(0);
        tm* lt = localtime(&now);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", lt);
        return string(buf);
    }

    TaskNode* findById(int id)
    {
        TaskNode* cur = head;
        while (cur) { if (cur->id == id) return cur; cur = cur->next; }
        return nullptr;
    }

    bool priorityTaken(int p)
    {
        TaskNode* cur = head;
        while (cur)
        {
            if (!cur->done && cur->priority == p) return true;
            cur = cur->next;
        }
        return false;
    }

    void cascadeDown(int p)
    {
        if (p > 10) return;
        TaskNode* target = nullptr;
        TaskNode* cur    = head;
        while (cur)
        {
            if (!cur->done && cur->priority == p) { target = cur; break; }
            cur = cur->next;
        }
        if (!target || p >= 10) return;
        if (priorityTaken(p + 1)) cascadeDown(p + 1);

        target->priority++;
        heap.updatePriority(target->id, target->priority);
        cout << "    -> \"" << target->title.substr(0, 28)
             << "\" shifted to P" << target->priority << "\n";
    }

    void appendNode(TaskNode* node)
    {
        node->next = nullptr;
        if (!head) { head = node; return; }
        TaskNode* cur = head;
        while (cur->next) cur = cur->next;
        cur->next = node;
    }

    void printDivider() { cout << "  " << string(72, '-') << "\n"; }

    string P(int p) { return "P" + to_string(p); }

public:
    TodoList() : head(nullptr), idCounter(1) {}

    void addTask(const string& title, int priority)
    {
        if (title.empty())
            { cout << "\n  [!] Title cannot be empty.\n"; return; }
        if (priority < 1 || priority > 10)
            { cout << "\n  [!] Priority must be 1-10.\n"; return; }

        if (priorityTaken(priority))
        {
            cout << "\n  [~] P" << priority
                 << " is occupied. Cascading existing tasks down:\n";
            cascadeDown(priority);
        }

        TaskNode* node  = new TaskNode();
        node->id        = idCounter++;
        node->title     = title;
        node->done      = false;
        node->priority  = priority;
        node->inPlanner = false;
        node->createdAt = currentTime();

        appendNode(node);
        heap.insert(priority, node->id);

        cout << "\n  [+] Task added  (ID: " << node->id
             << "  Priority: P" << node->priority << ")\n";
    }

    void markDone(int id)
    {
        TaskNode* t = findById(id);
        if (!t)     { cout << "\n  [!] Task ID " << id << " not found.\n"; return; }
        if (t->done){ cout << "\n  [!] Task already done.\n"; return; }
        t->done = true;
        heap.removeById(id);
        if (t->inPlanner) { planner.remove(id); t->inPlanner = false; }
        cout << "\n  [v] Task " << id << " marked as done!\n";
    }

    void deleteTask(int id)
    {
        TaskNode* cur = head, *prev = nullptr;
        while (cur)
        {
            if (cur->id == id)
            {
                undoStack.push(cur);
                heap.removeById(id);
                if (cur->inPlanner) planner.remove(id);
                if (prev) prev->next = cur->next;
                else      head       = cur->next;
                delete cur;
                cout << "\n  [-] Task " << id << " deleted. (Undo available)\n";
                return;
            }
            prev = cur; cur = cur->next;
        }
        cout << "\n  [!] Task ID " << id << " not found.\n";
    }

    void display()
    {
        if (!head) { cout << "\n  (no tasks yet)\n"; return; }
        cout << "\n";
        printDivider();
        cout << "  " << left
             << setw(5)  << "ID"
             << setw(30) << "TITLE"
             << setw(6)  << "PRI"
             << setw(11) << "STATUS"
             << "CREATED\n";
        printDivider();
        TaskNode* cur = head;
        while (cur)
        {
            string status = cur->done      ? "[Done]"    :
                            cur->inPlanner ? "[Planned]" : "[Todo]";
            cout << "  "
                 << setw(5)  << cur->id
                 << setw(30) << cur->title.substr(0, 28)
                 << setw(6)  << P(cur->priority)
                 << setw(11) << status
                 << cur->createdAt << "\n";
            cur = cur->next;
        }
        printDivider();
    }

    void search(const string& keyword)
    {
        bool found = false;
        cout << "\n  Search results for \"" << keyword << "\":\n";
        printDivider();
        TaskNode* cur = head;
        while (cur)
        {
            if (cur->title.find(keyword) != string::npos)
            {
                string status = cur->done ? "[Done]" : "[Todo]";
                cout << "  ID:" << setw(4) << cur->id
                     << "  " << setw(4) << P(cur->priority)
                     << "  " << cur->title
                     << "  " << status << "\n";
                found = true;
            }
            cur = cur->next;
        }
        if (!found) cout << "  No matching tasks found.\n";
        printDivider();
    }

    void undoDelete()
    {
        if (undoStack.isEmpty())
            { cout << "\n  [!] Nothing to undo.\n"; return; }

        TaskNode* r  = undoStack.pop();
        r->next      = nullptr;
        r->inPlanner = false;

        if (!r->done && priorityTaken(r->priority))
        {
            cout << "\n  [~] P" << r->priority
                 << " now occupied. Cascading to free slot:\n";
            cascadeDown(r->priority);
        }

        appendNode(r);
        if (!r->done) heap.insert(r->priority, r->id);
        cout << "\n  [<] \"" << r->title
             << "\" restored at P" << r->priority << "!\n";
    }

    void suggestNext()
    {
        if (heap.isEmpty())
            { cout << "\n  Great job! No pending tasks left.\n"; return; }

        HeapEntry top = heap.peekMin();
        TaskNode* t   = findById(top.taskId);
        if (!t) { cout << "\n  [!] Sync error.\n"; return; }

        cout << "\n  +------------------------------------------+\n";
        cout << "  |  SUGGESTED NEXT TASK                     |\n";
        cout << "  +------------------------------------------+\n";
        cout << "  |  ID       : " << setw(29) << left << t->id                  << "|\n";
        cout << "  |  Task     : " << setw(29) << left << t->title.substr(0, 28) << "|\n";
        cout << "  |  Priority : " << setw(29) << left << P(t->priority)          << "|\n";
        cout << "  |  Added    : " << setw(29) << left << t->createdAt            << "|\n";
        cout << "  +------------------------------------------+\n";
    }

    void displayByPriority()
    {
        HeapEntry sorted[500]; int sz = 0;
        heap.sortedSnapshot(sorted, sz);

        cout << "\n  Tasks sorted by Priority (pending only):\n";
        printDivider();
        cout << "  " << left
             << setw(6)  << "PRI"
             << setw(5)  << "ID"
             << setw(30) << "TITLE"
             << "CREATED\n";
        printDivider();
        if (sz == 0)
            cout << "  All tasks completed! Nothing pending.\n";
        else
            for (int i = 0; i < sz; i++)
            {
                TaskNode* t = findById(sorted[i].taskId);
                if (t)
                    cout << "  "
                         << setw(6)  << P(t->priority)
                         << setw(5)  << t->id
                         << setw(30) << t->title.substr(0, 28)
                         << t->createdAt << "\n";
            }
        printDivider();
    }

    void changePriority(int id, int newP)
    {
        TaskNode* t = findById(id);
        if (!t)      { cout << "\n  [!] Task ID " << id << " not found.\n"; return; }
        if (t->done) { cout << "\n  [!] Cannot change priority of a done task.\n"; return; }
        if (newP < 1 || newP > 10) { cout << "\n  [!] Priority must be 1-10.\n"; return; }

        int oldP = t->priority;
        t->priority = 999;
        heap.updatePriority(id, 999);

        if (priorityTaken(newP))
        {
            cout << "\n  [~] P" << newP << " is occupied. Cascading:\n";
            cascadeDown(newP);
        }

        t->priority = newP;
        heap.updatePriority(id, newP);
        cout << "\n  [*] Task " << id
             << "  P" << oldP << " -> P" << newP << "\n";
    }

    void addToPlanner(int id)
    {
        TaskNode* t = findById(id);
        if (!t)           { cout << "\n  [!] Task ID " << id << " not found.\n"; return; }
        if (t->done)      { cout << "\n  [!] Cannot plan a completed task.\n"; return; }
        if (t->inPlanner) { cout << "\n  [!] Task already in today's planner.\n"; return; }

        if (planner.enqueue(id))
        {
            t->inPlanner = true;
            cout << "\n  [>] \"" << t->title
                 << "\" added to planner  (Queue position: "
                 << planner.size() << ")\n";
        }
        else cout << "\n  [!] Planner is full.\n";
    }

    void displayPlanner()
    {
        cout << "\n  Today's Planner  (FIFO Queue  |  "
             << planner.size() << " tasks):\n";
        printDivider();

        if (planner.isEmpty())
        {
            cout << "  Queue is empty. Add tasks with option 11.\n";
            printDivider();
            return;
        }

        TaskNode* current = findById(planner.peek());
        if (current)
        {
            cout << "  FOCUS NOW  ->  P" << current->priority
                 << "  [ID:" << current->id << "]  "
                 << current->title << "\n";
            printDivider();
        }

        int ids[100]; int cnt = 0;
        planner.getAll(ids, cnt);
        for (int i = 1; i < cnt; i++)
        {
            TaskNode* t = findById(ids[i]);
            if (t)
                cout << "  " << setw(3) << (i + 1) << ".  P"
                     << t->priority << "  [ID:" << t->id << "]  "
                     << t->title << "\n";
        }
        printDivider();
    }

    void plannerDoneNext()
    {
        if (planner.isEmpty())
            { cout << "\n  [!] Planner is empty.\n"; return; }

        int id        = planner.dequeue();
        TaskNode* t   = findById(id);
        if (t)
        {
            t->done      = true;
            t->inPlanner = false;
            heap.removeById(id);
            cout << "\n  [v] \"" << t->title << "\" marked done!\n";
        }

        if (!planner.isEmpty())
        {
            TaskNode* next = findById(planner.peek());
            if (next)
                cout << "  [>] Next up: P" << next->priority
                     << "  \"" << next->title << "\"\n";
        }
        else cout << "  [*] Planner complete for today!\n";
    }

    void plannerSkip()
    {
        if (planner.isEmpty())
            { cout << "\n  [!] Planner is empty.\n"; return; }
        if (planner.size() == 1)
            { cout << "\n  [!] Only one task -- cannot skip.\n"; return; }

        int id = planner.dequeue();
        planner.enqueue(id);   
        TaskNode* t = findById(id);
        if (t) t->inPlanner = true;

        TaskNode* next = findById(planner.peek());
        if (next)
            cout << "\n  [>] Skipped. Next up: P" << next->priority
                 << "  \"" << next->title << "\"\n";
    }

    void showStats()
    {
        int total = 0, done = 0, pending = 0;
        TaskNode* cur = head;
        while (cur)
        {
            total++;
            if (cur->done) done++; else pending++;
            cur = cur->next;
        }
        cout << "\n  " << string(40, '-') << "\n";
        cout << "  Total tasks       : " << total         << "\n";
        cout << "  Completed         : " << done          << "\n";
        cout << "  Pending           : " << pending       << "\n";
        cout << "  In planner today  : " << planner.size()<< "\n";
        cout << "  In heap (pending) : " << heap.size()   << "\n";
        cout << "  " << string(40, '-') << "\n";
    }

    ~TodoList()
    {
        TaskNode* cur = head;
        while (cur) { TaskNode* t = cur; cur = cur->next; delete t; }
    }
};

void showMenu()
{
    cout << "\n";
    cout << "  +==========================================+\n";
    cout << "  |          CLI TO-DO LIST APP             |\n";
    cout << "  |  Linked List | Stack | Heap | Queue     |\n";
    cout << "  +==========================================+\n";
    cout << "  |  -- Linked List --                      |\n";
    cout << "  |   1. View all tasks                     |\n";
    cout << "  |   2. Add task                           |\n";
    cout << "  |   3. Mark task as done                  |\n";
    cout << "  |   4. Delete task                        |\n";
    cout << "  |   5. Search tasks                       |\n";
    cout << "  |  -- Stack --                            |\n";
    cout << "  |   6. Undo last delete                   |\n";
    cout << "  |  -- Min-Heap (Priority Queue) --        |\n";
    cout << "  |   7. View tasks by priority             |\n";
    cout << "  |   8. What should I do next?             |\n";
    cout << "  |   9. Change task priority               |\n";
    cout << "  |  -- Queue (Daily Planner) --            |\n";
    cout << "  |  10. View today's planner               |\n";
    cout << "  |  11. Add task to today's planner        |\n";
    cout << "  |  12. Mark current done & go next        |\n";
    cout << "  |  13. Skip current task (move to back)   |\n";
    cout << "  |  -- General --                          |\n";
    cout << "  |  14. Show stats                         |\n";
    cout << "  |   0. Exit                               |\n";
    cout << "  +==========================================+\n";
    cout << "  Choose: ";
}

int getPriority()
{
    int p;
    cout << "  Priority (1 = most urgent, 10 = least urgent): ";
    cin >> p; cin.ignore();
    if (p < 1 || p > 10) { cout << "  [!] Invalid. Defaulting to 5.\n"; return 5; }
    return p;
}

int main()
{
    TodoList list;
    list.addTask("Complete DSA mini project",         1);
    list.addTask("Submit assignment before deadline", 2);
    list.addTask("Study Linked Lists chapter",        4);
    list.addTask("Review sorting algorithms",         7);
    list.addTask("Fix bug in priority queue",         4); 
    list.addTask("Read about Min-Heap theory",        6);

    int choice;
    while (true)
    {
        showMenu();
        cin >> choice; cin.ignore();

        switch (choice)
        {
        case 1:  list.display(); break;

        case 2:
        {
            cout << "  Task title: ";
            string title; getline(cin, title);
            int p = getPriority();
            list.addTask(title, p);
            break;
        }
        case 3:
        {
            cout << "  Enter Task ID to mark done: ";
            int id; cin >> id; cin.ignore();
            list.markDone(id);
            break;
        }
        case 4:
        {
            cout << "  Enter Task ID to delete: ";
            int id; cin >> id; cin.ignore();
            list.deleteTask(id);
            break;
        }
        case 5:
        {
            cout << "  Search keyword: ";
            string kw; getline(cin, kw);
            list.search(kw);
            break;
        }

        case 6:  list.undoDelete(); break;

        case 7:  list.displayByPriority(); break;
        case 8:  list.suggestNext();       break;
        case 9:
        {
            cout << "  Enter Task ID to update: ";
            int id; cin >> id; cin.ignore();
            int p = getPriority();
            list.changePriority(id, p);
            break;
        }

        case 10: list.displayPlanner(); break;
        case 11:
        {
            cout << "  Enter Task ID to add to planner: ";
            int id; cin >> id; cin.ignore();
            list.addToPlanner(id);
            break;
        }
        case 12: list.plannerDoneNext(); break;
        case 13: list.plannerSkip();     break;

        case 14: list.showStats(); break;

        case 0:
            cout << "\n  Goodbye! Keep completing those tasks :)\n\n";
            return 0;

        default:
            cout << "\n  [!] Invalid option. Try again.\n";
        }
    }
}