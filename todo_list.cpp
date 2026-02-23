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
    string    createdAt;
    TaskNode* next;
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
        *copy = *t;
        copy->next = nullptr;

        StackNode* sn = new StackNode();
        sn->task = copy;
        sn->next = top;
        top = sn;
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

    bool isEmpty() { return top == nullptr; }

    ~UndoStack()
    {
        while (!isEmpty())
        {
            TaskNode* t = pop();
            delete t;
        }
    }
};

struct HeapEntry
{
    int priority;
    int taskId;
};

class PriorityQueue
{
    static const int MAX_SIZE = 500;
    HeapEntry heap[MAX_SIZE];
    int       sz;

    int parent(int i) { return (i - 1) / 2; }
    int left(int i)   { return 2 * i + 1;   }
    int right(int i)  { return 2 * i + 2;   }

    void swapEntries(int i, int j)
    {
        HeapEntry tmp = heap[i];
        heap[i] = heap[j];
        heap[j] = tmp;
    }

    void heapifyUp(int i)
    {
        while (i > 0 && heap[i].priority < heap[parent(i)].priority)
        {
            swapEntries(i, parent(i));
            i = parent(i);
        }
    }

    void heapifyDown(int i)
    {
        int smallest = i;
        int l = left(i);
        int r = right(i);

        if (l < sz && heap[l].priority < heap[smallest].priority)
            smallest = l;
        if (r < sz && heap[r].priority < heap[smallest].priority)
            smallest = r;

        if (smallest != i)
        {
            swapEntries(i, smallest);
            heapifyDown(smallest);
        }
    }

public:
    PriorityQueue() : sz(0) {}

    void insert(int priority, int taskId)
    {
        if (sz >= MAX_SIZE) { cout << "  [!] Priority queue is full.\n"; return; }
        heap[sz] = {priority, taskId};
        heapifyUp(sz);
        sz++;
    }

    HeapEntry peekMin() const { return heap[0]; }

    HeapEntry extractMin()
    {
        HeapEntry top = heap[0];
        heap[0] = heap[sz - 1];
        sz--;
        heapifyDown(0);
        return top;
    }

    void removeById(int taskId)
    {
        int idx = -1;
        for (int i = 0; i < sz; i++)
            if (heap[i].taskId == taskId) { idx = i; break; }
        if (idx == -1) return;

        heap[idx] = heap[sz - 1];
        sz--;
        if (idx < sz) { heapifyUp(idx); heapifyDown(idx); }
    }

    void updatePriority(int taskId, int newPriority)
    {
        for (int i = 0; i < sz; i++)
        {
            if (heap[i].taskId == taskId)
            {
                heap[i].priority = newPriority;
                heapifyUp(i);
                heapifyDown(i);
                return;
            }
        }
    }

    bool isEmpty() const { return sz == 0; }
    int  getSize() const { return sz; }

    void getSortedSnapshot(HeapEntry* out, int& outSize) const
    {
        outSize = sz;
        for (int i = 0; i < sz; i++) out[i] = heap[i];

        for (int i = 0; i < outSize - 1; i++)
        {
            int minIdx = i;
            for (int j = i + 1; j < outSize; j++)
                if (out[j].priority < out[minIdx].priority)
                    minIdx = j;
            HeapEntry tmp = out[i];
            out[i]        = out[minIdx];
            out[minIdx]   = tmp;
        }
    }
};

class TodoList
{
    TaskNode*     head;
    int           idCounter;
    UndoStack     undoStack;
    PriorityQueue pq;

    string currentTime()
    {
        time_t now = time(0);
        tm*    ltm = localtime(&now);
        char   buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", ltm);
        return string(buf);
    }

    TaskNode* findById(int id)
    {
        TaskNode* cur = head;
        while (cur) { if (cur->id == id) return cur; cur = cur->next; }
        return nullptr;
    }

    bool priorityTaken(int priority)
    {
        TaskNode* cur = head;
        while (cur)
        {
            if (!cur->done && cur->priority == priority) return true;
            cur = cur->next;
        }
        return false;
    }

    void cascadeDown(int priority)
    {
        if (priority > 10) return;

        TaskNode* target = nullptr;
        TaskNode* cur    = head;
        while (cur)
        {
            if (!cur->done && cur->priority == priority)
            {
                target = cur;
                break;
            }
            cur = cur->next;
        }

        if (!target)        return; 
        if (priority >= 10) return; 

        if (priorityTaken(priority + 1))
            cascadeDown(priority + 1);

        target->priority++;
        pq.updatePriority(target->id, target->priority);
        cout << "    -> \"" << target->title.substr(0, 25)
             << "\" shifted to P" << target->priority << "\n";
    }

public:
    TodoList() : head(nullptr), idCounter(1) {}

    void addTask(const string& title, int priority)
    {
        if (priorityTaken(priority))
        {
            cout << "\n  [~] P" << priority
                 << " is occupied. Cascading existing tasks down:\n";
            cascadeDown(priority);
        }

        TaskNode* newNode  = new TaskNode();
        newNode->id        = idCounter++;
        newNode->title     = title;
        newNode->done      = false;
        newNode->priority  = priority;
        newNode->createdAt = currentTime();
        newNode->next      = nullptr;

        if (!head) head = newNode;
        else
        {
            TaskNode* cur = head;
            while (cur->next) cur = cur->next;
            cur->next = newNode;
        }

        pq.insert(priority, newNode->id);
        cout << "\n  [+] Task added  (ID: " << newNode->id
             << ", Priority: P" << newNode->priority << ")\n";
    }

    void markDone(int id)
    {
        TaskNode* t = findById(id);
        if (!t)     { cout << "\n  [!] Task ID " << id << " not found.\n"; return; }
        if (t->done){ cout << "\n  [!] Task already marked as done.\n";    return; }
        t->done = true;
        pq.removeById(id);   
        cout << "\n  [v] Task " << id << " marked as done!\n";
    }

    void deleteTask(int id)
    {
        TaskNode* cur  = head;
        TaskNode* prev = nullptr;
        while (cur)
        {
            if (cur->id == id)
            {
                undoStack.push(cur);
                pq.removeById(id);
                if (prev) prev->next = cur->next;
                else      head       = cur->next;
                delete cur;
                cout << "\n  [-] Task " << id << " deleted. (Undo available)\n";
                return;
            }
            prev = cur;
            cur  = cur->next;
        }
        cout << "\n  [!] Task ID " << id << " not found.\n";
    }

    void undoDelete()
    {
        if (undoStack.isEmpty()) { cout << "\n  [!] Nothing to undo.\n"; return; }

        TaskNode* restored = undoStack.pop();
        restored->next = nullptr;

        if (!restored->done && priorityTaken(restored->priority))
        {
            cout << "\nP" << restored->priority
                 << " is now occupied. Cascading to restore original slot:\n";
            cascadeDown(restored->priority);
        }

        if (!head) head = restored;
        else
        {
            TaskNode* cur = head;
            while (cur->next) cur = cur->next;
            cur->next = restored;
        }

        if (!restored->done)
            pq.insert(restored->priority, restored->id);

        cout << "\nTask \"" << restored->title
             << "\" restored at P" << restored->priority << "!\n";
    }

    void display()
    {
        if (!head) { cout << "\n  (no tasks yet)\n"; return; }

        cout << "\n";
        cout << "  " << string(72, '-') << "\n";
        cout << "  " << left
             << setw(5)  << "ID"
             << setw(30) << "TITLE"
             << setw(6)  << "PRI"
             << setw(10) << "STATUS"
             << "CREATED\n";
        cout << "  " << string(72, '-') << "\n";

        TaskNode* cur = head;
        while (cur)
        {
            string status = cur->done ? "[Done]" : "[Todo]";
            string pri    = "P" + to_string(cur->priority);
            cout << "  "
                 << setw(5)  << cur->id
                 << setw(30) << cur->title.substr(0, 28)
                 << setw(6)  << pri
                 << setw(10) << status
                 << cur->createdAt << "\n";
            cur = cur->next;
        }
        cout << "  " << string(72, '-') << "\n";
    }

    void displayByPriority()
    {
        if (!head) { cout << "\n  (no tasks yet)\n"; return; }

        HeapEntry sorted[500];
        int       sortedSize = 0;
        pq.getSortedSnapshot(sorted, sortedSize);

        cout << "\n  Tasks sorted by Priority (pending only):\n";
        cout << "  " << string(72, '-') << "\n";
        cout << "  " << left
             << setw(5)  << "ID"
             << setw(30) << "TITLE"
             << setw(6)  << "PRI"
             << "CREATED\n";
        cout << "  " << string(72, '-') << "\n";

        if (sortedSize == 0)
            cout << "  All tasks are completed! Nothing pending.\n";
        else
            for (int i = 0; i < sortedSize; i++)
            {
                TaskNode* t = findById(sorted[i].taskId);
                if (t)
                {
                    string pri = "P" + to_string(t->priority);
                    cout << "  "
                         << setw(5)  << t->id
                         << setw(30) << t->title.substr(0, 28)
                         << setw(6)  << pri
                         << t->createdAt << "\n";
                }
            }
        cout << "  " << string(72, '-') << "\n";
    }

    void suggestNext()
    {
        if (pq.isEmpty()) { cout << "\n  Great job! No pending tasks left.\n"; return; }

        HeapEntry top = pq.peekMin();
        TaskNode* t   = findById(top.taskId);
        if (!t)   { cout << "\n  [!] Sync error.\n"; return; }

        string pri = "P" + to_string(t->priority);
        cout << "\n  +------------------------------------------+\n";
        cout << "  |  SUGGESTED NEXT TASK                     |\n";
        cout << "  +------------------------------------------+\n";
        cout << "  |  ID       : " << setw(29) << left << t->id                  << "|\n";
        cout << "  |  Task     : " << setw(29) << left << t->title.substr(0, 28) << "|\n";
        cout << "  |  Priority : " << setw(29) << left << pri                    << "|\n";
        cout << "  |  Added    : " << setw(29) << left << t->createdAt            << "|\n";
        cout << "  +------------------------------------------+\n";
    }

    void changePriority(int id, int newPriority)
    {
        TaskNode* t = findById(id);
        if (!t)     { cout << "\n  [!] Task ID " << id << " not found.\n"; return; }
        if (t->done){ cout << "\n  [!] Cannot change priority of a completed task.\n"; return; }

        int oldPriority = t->priority;

        pq.updatePriority(id, 999);
        t->priority = 999;

        if (priorityTaken(newPriority))
        {
            cout << "\n  [~] P" << newPriority
                 << " is occupied. Cascading existing tasks down:\n";
            cascadeDown(newPriority);
        }

        t->priority = newPriority;
        pq.updatePriority(id, newPriority);

        cout << "\n  [*] Task " << id
             << " priority: P" << oldPriority
             << " -> P"        << newPriority << "\n";
    }

    void search(const string& keyword)
    {
        bool found = false;
        TaskNode* cur = head;
        cout << "\n  Search results for \"" << keyword << "\":\n";
        cout << "  " << string(72, '-') << "\n";
        while (cur)
        {
            if (cur->title.find(keyword) != string::npos)
            {
                string status = cur->done ? "[Done]" : "[Todo]";
                string pri    = "P" + to_string(cur->priority);
                cout << "  ID:" << setw(4) << cur->id
                     << "  "   << setw(6)  << pri
                     << "  "   << cur->title
                     << "  "   << status << "\n";
                found = true;
            }
            cur = cur->next;
        }
        if (!found) cout << "  No matching tasks found.\n";
        cout << "  " << string(72, '-') << "\n";
    }

    void showStats()
    {
        int total = 0, done = 0, pending = 0;
        TaskNode* cur = head;
        while (cur)
        {
            total++;
            if (cur->done) done++;
            else pending++;
            cur = cur->next;
        }
        cout << "\n  " << string(40, '-') << "\n";
        cout << "  Total            : " << total   << "\n";
        cout << "  Done             : " << done    << "\n";
        cout << "  Pending          : " << pending << "\n";
        cout << "  Pending in heap  : " << pq.getSize() << "\n";
        cout << "  " << string(40, '-') << "\n";
    }

    ~TodoList()
    {
        TaskNode* cur = head;
        while (cur) { TaskNode* tmp = cur; cur = cur->next; delete tmp; }
    }
};

void showMenu()
{
    cout << "\n";
    cout << "  +======================================+\n";
    cout << "  |       CLI TO-DO LIST APP             |\n";
    cout << "  |  Linked List + Stack + Min-Heap      |\n";
    cout << "  +======================================+\n";
    cout << "  |  1. View all tasks                   |\n";
    cout << "  |  2. Add task                         |\n";
    cout << "  |  3. Mark task as done                |\n";
    cout << "  |  4. Delete task                      |\n";
    cout << "  |  5. Undo last delete                 |\n";
    cout << "  |  6. Search tasks                     |\n";
    cout << "  |  7. Show stats                       |\n";
    cout << "  |  8. View tasks by priority           |\n";
    cout << "  |  9. What should I do next?           |\n";
    cout << "  | 10. Change task priority             |\n";
    cout << "  |  0. Exit                             |\n";
    cout << "  +======================================+\n";
    cout << "  Choose: ";
}

int getPriorityInput()
{
    int p;
    cout << "  Priority (1=most urgent  to  10=least urgent): ";
    cin >> p;
    cin.ignore();
    if (p < 1 || p > 10)
    {
        cout << "  [!] Invalid. Defaulting to 5.\n";
        p = 5;
    }
    return p;
}

int main()
{
    TodoList list;
    int choice;

    list.addTask("Complete mini project",             1);
    list.addTask("Submit assignment before deadline", 2);
    list.addTask("Read DSA chapter on Linked Lists",  4);
    list.addTask("Review sorting algorithms",         7);
    list.addTask("Fix bug in priority queue",         4); 

    while (true)
    {
        showMenu();
        cin >> choice;
        cin.ignore();

        switch (choice)
        {
            case 1:  list.display();          
            break;

            case 2:
            {
                cout << "  Task title: ";
                string title;
                getline(cin, title);
                if (title.empty()) 
                { cout << "  [!] Title cannot be empty.\n"; 
                    break; 
                }
                int p = getPriorityInput();
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

            case 5:  list.undoDelete();       
            break;

            case 6:
            {
                cout << "  Search keyword: ";
                string kw;
                getline(cin, kw);
                list.search(kw);
                break;
            }

            case 7:  list.showStats();          
            break;
            case 8:  list.displayByPriority();  
            break;
            case 9:  list.suggestNext();         
            break;

            case 10:
            {
                cout << "  Enter Task ID to update: ";
                int id; cin >> id; cin.ignore();
                int p = getPriorityInput();
                list.changePriority(id, p);
                break;
            }

            case 0:
                cout << "\n  Goodbye! Keep completing those tasks :)\n\n";
                return 0;

            default:
                cout << "\n  [!] Invalid option. Try again.\n";
        }
    }
}