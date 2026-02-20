#include <iostream>
#include <string>
#include <ctime>
#include <iomanip>
using namespace std;

struct TaskNode 
{
    int id;
    string title;
    bool done;
    string createdAt;
    TaskNode* next;
};

struct StackNode 
{
    TaskNode* task;   
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
        StackNode* tmp = top;
        TaskNode*  task = tmp->task;
        top = top->next;
        delete tmp;
        return task; 
    }

    bool isEmpty() 
    { 
        return top == nullptr;
    }

    ~UndoStack() 
    {
        while (!isEmpty()) 
        {
            TaskNode* t = pop();
            delete t;
        }
    }
};

class TodoList 
{
    TaskNode* head;
    int       idCounter;
    UndoStack undoStack;

    string currentTime() 
    {
        time_t now = time(0);
        tm*    ltm = localtime(&now);
        char   buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M", ltm);
        return string(buf);
    }

public:
    TodoList() : head(nullptr), idCounter(1) {}

    void addTask(const string& title) 
    {
        TaskNode* newNode  = new TaskNode();
        newNode->id        = idCounter++;
        newNode->title     = title;
        newNode->done      = false;
        newNode->createdAt = currentTime();
        newNode->next      = nullptr;

        if (!head) 
        {
            head = newNode;
        } 
        else 
        {
            TaskNode* cur = head;
            while (cur->next) cur = cur->next;
            cur->next = newNode;
        }
        cout << "\nTask added  (ID: " << newNode->id << ")\n";
    }

    void markDone(int id) 
    {
        TaskNode* cur = head;
        while (cur) 
        {
            if (cur->id == id) 
            {
                if (cur->done) 
                {
                    cout << "\n Task already marked as done.\n";
                } 
                else 
                {
                    cur->done = true;
                    cout << "\n Task " << id << " marked as done!\n";
                }
                return;
            }
            cur = cur->next;
        }
        cout << "\nTask with ID " << id << " not found.\n";
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

                if (prev) prev->next = cur->next;
                else       head      = cur->next;

                delete cur;
                cout << "\nTask " << id << " deleted. (Undo available)\n";
                return;
            }
            prev = cur;
            cur  = cur->next;
        }
        cout << "\nTask with ID " << id << " not found.\n";
    }

    void undoDelete() 
    {
        if (undoStack.isEmpty()) 
        {
            cout << "\n Nothing to undo.\n";
            return;
        }
        TaskNode* restored = undoStack.pop();
        restored->next = nullptr;

        
        if (!head) 
        {
            head = restored;
        } else 
        {
            TaskNode* cur = head;
            while (cur->next) cur = cur->next;
            cur->next = restored;
        }
        cout << "\nTask \"" << restored->title << "\" restored!\n";
    }

    void display() 
    {
        if (!head) 
        {
            cout << "\n  (no tasks yet)\n";
            return;
        }

        cout << "\n";
        cout << "  " << string(60, '-') << "\n";
        cout << "  " << left
             << setw(5)  << "ID"
             << setw(30) << "TITLE"
             << setw(10) << "STATUS"
             << "CREATED\n";
        cout << "  " << string(60, '-') << "\n";

        TaskNode* cur = head;
        while (cur) 
        {
            string status = cur->done ? "[Done]" : "[Todo]";
            cout << "  "
                 << setw(5)  << cur->id
                 << setw(30) << cur->title.substr(0, 28)
                 << setw(10) << status
                 << cur->createdAt << "\n";
            cur = cur->next;
        }
        cout << "  " << string(60, '-') << "\n";
    }

    void search(const string& keyword) 
    {
        bool found = false;
        TaskNode* cur = head;
        cout << "\n  Search results for \"" << keyword << "\":\n";
        cout << "  " << string(60, '-') << "\n";
        while (cur) 
        {
            if (cur->title.find(keyword) != string::npos) 
            {
                string status = cur->done ? "[Done]" : "[Todo]";
                cout << "  ID:" << cur->id << "  " << cur->title
                     << "  " << status << "\n";
                found = true;
            }
            cur = cur->next;
        }
        if (!found) cout << "  No matching tasks found.\n";
        cout << "  " << string(60, '-') << "\n";
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
        cout << "\n  Total: " << total << " Done: " << done << " Pending: " << pending << "\n";
    }

    ~TodoList() 
    {
        TaskNode* cur = head;
        while (cur) 
        {
            TaskNode* tmp = cur;
            cur = cur->next;
            delete tmp;
        }
    }
};

void showMenu() 
{
    cout << "\n";
    cout << "CLI TO-DO LIST APP\n";
    cout << "(Linked List + Stack | DSA)\n";
    cout << "1. View all tasks\n";
    cout << "2. Add task\n";
    cout << "3. Mark task as done\n";
    cout << "4. Delete task\n";
    cout << "5. Undo last delete\n";
    cout << "6. Search tasks\n";
    cout << "7. Show stats\n";
    cout << "0. Exit\n";
    cout << "Choose:";
}

int main() 
{
    TodoList list;
    int choice;

    list.addTask("Read DSA chapter on Linked Lists");
    list.addTask("Complete mini project");
    list.addTask("Submit assignment before deadline");

    while (true) 
    {
        showMenu();
        cin >> choice;
        cin.ignore();   

        switch (choice) 
        {
            case 1:
                list.display();
                break;

            case 2: 
            {
                cout << "  Task title: ";
                string title;
                getline(cin, title);
                if (!title.empty()) list.addTask(title);
                else cout << "Title cannot be empty.\n";
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
                list.undoDelete();
                break;

            case 6: 
            {
                cout << "  Search keyword: ";
                string kw;
                getline(cin, kw);
                list.search(kw);
                break;
            }

            case 7:
                list.showStats();
                break;

            case 0:
                cout << "\n  Goodbye! Keep completing those tasks :)\n\n";
                return 0;

            default:
                cout << "\nInvalid option. Try again.\n";
        }
    }
}