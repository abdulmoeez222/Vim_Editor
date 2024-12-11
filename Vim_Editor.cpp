#include <iostream>
#include <vector>
#include <cstdlib>
#include <iomanip>
#include <fstream>
#include <sstream>
#include <string>
#ifdef _WIN32
#include <conio.h>
#elif defined(linux) || defined(APPLE)
#include <termios.h>
#include <unistd.h>
#endif
using namespace std;


struct EditorStatus {
	string currentMode;     // "INSERT" or "NORMAL"
	size_t cursorLine;      // Current line number
	size_t cursorColumn;    // Current column number
	size_t totalLines;      // Total lines in the document
	string lastCommand;     // Last executed command
};
struct node {
public:
	char data;
	node* next;
	node* previous;
	node(char a) : data(a), next(nullptr), previous(nullptr) {}
};
class SearchEngine {
public:
	string lastPattern;
	size_t lastMatchLine;
	size_t lastMatchColumn;
	SearchEngine() : lastMatchLine(0), lastMatchColumn(0) {}

	bool search(const vector<node*>& lines, const string& str) {
		lastPattern = str;
		for (size_t i = 0; i < lines.size(); ++i) {
			node* current = lines[i];
			size_t column = 0;
			while (current) {
				node* temp = current;
				size_t j = 0;
				while (temp && j < str.size() && temp->data == str[j]) {
					temp = temp->next;
					j++;
				}
				if (j == str.size()) { // Found a match
					lastMatchLine = i;
					lastMatchColumn = column;
					return true;
				}
				current = current->next;
				column++;
			}
		}
		lastMatchLine = 0;
		lastMatchColumn = 0;
		return false;
	}


	bool findNext(const vector<node*>& lines) {
		// Implement logic to find the next occurrence of lastPattern
		for (int i = lastMatchLine; i < lines.size(); ++i) {
			node* current = lines[i];
			int column = (i == lastMatchLine) ? lastMatchColumn : 0;
			while (current) {
				if (current->data == lastPattern[0]) {
					node* temp = current;
					size_t j = 0;
					while (temp && j < lastPattern.size() && temp->data == lastPattern[j]) {
						temp = temp->next;
						j++;
					}
					if (j == lastPattern.size()) {
						lastMatchLine = i;
						lastMatchColumn = column;
						return true;
					}
				}
				current = current->next;
				column++;
			}
		}
		return false; // No more occurrences found
	}

	bool findPrevious(const vector<node*>& lines) {
		for (size_t i = lastMatchLine; i < lines.size(); ++i) {
			node* current = lines[i];
			size_t column = (i == lastMatchLine) ? lastMatchColumn : 0;
			while (current) {
				if (current->data == lastPattern[0]) {
					node* temp = current;
					size_t j = 0;
					while (temp&& j < lastPattern.size() && temp->data == lastPattern[j]) {
						temp = temp->next;
						j++;
					}
					if (j == lastPattern.size()) {
						lastMatchLine = i;
						lastMatchColumn = column;
						return true;
					}
				}
				current = current->next;
				column++;
			}
		}

		// If we reach here, we need to check previous lines
		for (size_t i = lastMatchLine; i-- > 0;) {
			node* current = lines[i];
			size_t column = 0;
			while (current) {
				if (current->data == lastPattern[0]) {
					node* temp = current;
					size_t j = 0;
					while (temp && j < lastPattern.size() && temp->data == lastPattern[j]) {
						temp = temp->next;
						j++;
					}
					if (j == lastPattern.size()) {
						lastMatchLine = i;
						lastMatchColumn = column;
						return true;
					}
				}
				current = current->next;
				column++;
			}
		}
		return false; // No previous occurrences found
	}

	bool replace(vector<node*>& lines, const string& oldStr, const string& newStr, bool global = false) {
		if (lines.empty() || oldStr.empty()) return false;
		bool replaced = false;

		for (size_t i = 0; i < lines.size(); ++i) {
			node* current = lines[i];
			string lineContent;

			// Build string from linked list
			while (current) {
				lineContent += current->data;
				current = current->next;
			}

			size_t pos = lineContent.find(oldStr);
			if (pos != string::npos) {
				replaced = true;

				do {
					lineContent.replace(pos, oldStr.size(), newStr);
					pos = global ? lineContent.find(oldStr, pos + newStr.size()) : string::npos;
				} while (global && pos != string::npos);

				// Rebuild the linked list
				node* newLine = nullptr;
				node* prev = nullptr;
				for (char ch : lineContent) {
					node* newNode = new node(ch);
					if (!newLine) newLine = newNode;
					if (prev) prev->next = newNode;
					newNode->previous = prev;
					prev = newNode;
				}

				// Delete the old line and assign the new one
				current = lines[i];
				while (current) {
					node* toDelete = current;
					current = current->next;
					delete toDelete;
				}
				lines[i] = newLine;
			}
			if (!global && replaced) break;
		}
		return replaced;
	}

};
class FileManager {
private:
	string currentFileName;
	bool modified;

public:
	FileManager() : currentFileName(""), modified(false) {}

	bool loadFile(const string& filename, vector<node*>& lines) {
		ifstream file(filename);
		if (!file.is_open()) {
			return false;
		}
		lines.clear();
		string lineContent;
		while (getline(file, lineContent)) {
			node* lineStart = nullptr;
			node* current = nullptr;
			for (char ch : lineContent) {
				node* newNode = new node(ch);
				if (!lineStart) {
					lineStart = newNode;
				}
				else {
					current->next = newNode;
					newNode->previous = current;
				}
				current = newNode;
			}
			lines.push_back(lineStart);
		}
		file.close();
		currentFileName = filename;
		modified = false;
		return true;
	}

	bool saveFile(const string& filename, const vector<node*>& lines) {
		ofstream file(filename);
		if (!file.is_open()) {
			return false;
		}

		for (node* line : lines) {
			node* current = line;
			while (current) {
				file << current->data;
				current = current->next;
			}
			file << '\n';
		}
		file.close();
		currentFileName = filename;
		modified = false;
		return true;
	}

	void markAsModified() {
		modified = true;
	}
	bool hasUnsavedChanges() {
		return modified;
	}
	string getCurrentFileName() {
		return currentFileName;
	}
};

class TextEditor {
	vector<node*> lines;
	int current_line;
	node* Cursor;
	bool insertMode;
	string copyBuffer;
	EditorStatus status;
	FileManager fileManager;
	SearchEngine searchEngine;
	bool isWordCharacter(char c) {
		if ((c >= 65 && c <= 90) || (c >= 97 && c <= 122)) {
			return true;
		}
		return false;
	}
	bool isPunctuation(char c) {
		if ((c >= 33 && c <= 47) || (c >= 58 && c <= 64) || (c == ' ')) {
			return true;
		}
		return false;
	}

public:
	TextEditor() : current_line(0), Cursor(nullptr), insertMode(false) {
		lines.push_back(nullptr);
		updateStatus();
	}
	void joinLines() {
		if (current_line < lines.size() - 1) {
			node* currentLine = lines[current_line];
			node* nextLine = lines[current_line + 1];

			// Find the end of the current line
			while (currentLine && currentLine->next) {
				currentLine = currentLine->next;
			}

			// Link the last node of the current line to the first node of the next line
			if (currentLine) {
				currentLine->next = nextLine;
				if (nextLine) {
					nextLine->previous = currentLine;
				}
			}

			// Remove the next line from the vector
			lines.erase(lines.begin() + current_line + 1);
			markModified();
			updateStatus("Joined lines");
		}
	}

	void indentLine(bool increase) {
		node* currentLine = lines[current_line];
		if (!currentLine) return;

		// Indent or unindent by adding/removing spaces
		if (increase) {
			insert(' '); // Add a space at the beginning
		}
		else {
			// Remove the first character if it's a space
			if (currentLine->data == ' ') {
				deleteCharacterAtCursor(); // This will delete the first space
			}
		}
		markModified();
		updateStatus(increase ? "Indented line" : "Unindented line");
	}

	void deleteLineNumber(size_t lineNum) {
		if (lineNum < 1 || lineNum > lines.size()) {
			cout << "Invalid line number." << endl;
			return;
		}

		// Adjust for 0-based index
		lineNum--;

		// Delete the specified line
		node* currentLine = lines[lineNum];
		while (currentLine) {
			node* toDelete = currentLine;
			currentLine = currentLine->next;
			delete toDelete;
		}

		lines.erase(lines.begin() + lineNum);
		if (current_line >= lines.size()) {
			current_line = lines.size() - 1; // Adjust current line if needed
		}
		markModified();
		updateStatus("Deleted line " + to_string(lineNum + 1));
	}

	void executeWithCount(int count, const string& cmd) {
		if (cmd == "dd") {
			for (int i = 0; i < count; ++i) {
				deleteLineNumber(current_line + 1); // Delete the current line
			}
		}
		else if (cmd == "yy") {
			// Implement yank (copy) functionality for count lines
			// This is a placeholder; you would need to implement the actual yank logic
			cout << "Yanked " << count << " lines." << endl;
		}
		else if (cmd == "j") {
			for (int i = 0; i < count; ++i) {
				moveDown(); // Move down count lines
			}
		}
		else if (cmd == ">>") {
			for (int i = 0; i < count; ++i) {
				indentLine(true); // Indent current line
			}
		}
		else if (cmd == "<<") {
			for (int i = 0; i < count; ++i) {
				indentLine(false); // Unindent current line
			}
		}
	}
	void moveToColumn(size_t column) {
		Cursor = lines[current_line];
		for (size_t i = 0; i < column && Cursor != nullptr; ++i) {
			Cursor = Cursor->next;
		}
	}
	void search(const string& str) {
		if (searchEngine.search(lines, str)) {
			current_line = searchEngine.lastMatchLine;
			moveToColumn(searchEngine.lastMatchColumn);
			updateStatus("Search: " + str);
		}
		else {
			cout << "Pattern not found: " << str << endl;
		}
	}

	void findNext() {
		if (searchEngine.findNext(lines)) {
			current_line = searchEngine.lastMatchLine;
			moveToColumn(searchEngine.lastMatchColumn);
			updateStatus("Find Next");
		}
		else {
			cout << "No more occurrences found." << endl;
		}
	}


	void findPrevious() {
		if (searchEngine.findPrevious(lines)) {
			current_line = searchEngine.lastMatchLine;
			moveToColumn(searchEngine.lastMatchColumn);
			updateStatus("Find Previous");
		}
		else {
			cout << "No previous occurrences found." << endl;
		}
	}

	void replace(const string& oldStr, const string& newStr, bool global) {
		if (searchEngine.replace(lines, oldStr, newStr, global)) {
			updateStatus("Replaced: " + oldStr + " with " + newStr);
		}
		else {
			cout << "No occurrences of " << oldStr << " found in the current line." << endl;
		}
	}
	void replaceFirst(const string& oldText, const string& newText) {
		if (oldText.empty()) {
			updateStatus("Error: Search text cannot be empty.");
			return;
		}

		node* current = lines[current_line];
		size_t matchIndex = 0;
		node* matchStart = nullptr;

		// Traverse the linked list to find the first match
		while (current) {
			if (current->data == oldText[matchIndex]) {
				if (matchIndex == 0) matchStart = current;
				matchIndex++;

				if (matchIndex == oldText.length()) {
					// Match found; perform the replacement
					node* replaceCursor = matchStart;
					for (char ch : newText) {
						replaceCursor->data = ch;
						if (replaceCursor->next == nullptr && &ch != &newText.back()) {
							// Add new nodes if newText is longer than oldText
							node* newNode = new node('\0');
							replaceCursor->next = newNode;
							newNode->previous = replaceCursor;
						}
						replaceCursor = replaceCursor->next;
					}

					// Remove extra nodes if newText is shorter
					while (replaceCursor && matchIndex < oldText.length()) {
						node* toDelete = replaceCursor;
						replaceCursor = replaceCursor->next;
						delete toDelete;
						matchIndex++;
					}

					if (replaceCursor) replaceCursor->previous = matchStart;
					matchStart->next = replaceCursor;

					markModified();
					updateStatus("First occurrence replaced.");
					return;
				}
			}
			else {
				matchIndex = 0;
				matchStart = nullptr;
			}
			current = current->next;
		}

		updateStatus("No occurrences found to replace.");
	}

	void replaceAll(const string& oldText, const string& newText) {
		if (oldText.empty()) {
			updateStatus("Error: Search text cannot be empty.");
			return;
		}

		node* current = lines[current_line];
		size_t matchIndex = 0;
		node* matchStart = nullptr;
		bool found = false;

		// Traverse the linked list to find all matches
		while (current) {
			if (current->data == oldText[matchIndex]) {
				if (matchIndex == 0) matchStart = current;
				matchIndex++;

				if (matchIndex == oldText.length()) {
					// Match found; perform the replacement
					node* replaceCursor = matchStart;
					for (char ch : newText) {
						replaceCursor->data = ch;
						if (replaceCursor->next == nullptr && &ch != &newText.back()) {
							// Add new nodes if newText is longer than oldText
							node* newNode = new node('\0');
							replaceCursor->next = newNode;
							newNode->previous = replaceCursor;
						}
						replaceCursor = replaceCursor->next;
					}

					// Remove extra nodes if newText is shorter
					while (replaceCursor && matchIndex < oldText.length()) {
						node* toDelete = replaceCursor;
						replaceCursor = replaceCursor->next;
						delete toDelete;
						matchIndex++;
					}

					if (replaceCursor) replaceCursor->previous = matchStart;
					matchStart->next = replaceCursor;

					found = true;
					matchIndex = 0;
					matchStart = nullptr; // Reset for the next match
				}
			}
			else {
				matchIndex = 0;
				matchStart = nullptr;
			}
			current = current->next;
		}

		updateStatus(found ? "All occurrences replaced." : "No occurrences found to replace.");
		if (found) markModified();
	}


	void saveToFile(const string& filename) {
		if (fileManager.saveFile(filename, lines)) {
			cout << "File saved successfully to " << filename << "\n";
		}
		else {
			cout << "Failed to save file!\n";
		}
	}

	void loadFromFile(const string& filename) {
		if (fileManager.loadFile(filename, lines)) {
			current_line = 0;
			Cursor = lines.empty() ? nullptr : lines[0];
			updateStatus("File Loaded");
		}
		else {
			cout << "Failed to load file!\n";
		}
	}

	void markModified() {
		fileManager.markAsModified();
	}

	bool hasUnsavedChanges() {
		return fileManager.hasUnsavedChanges();
	}

	string getFileName() {
		return fileManager.getCurrentFileName();
	}
	size_t countCharactersInLine(node* lineStart) {
		size_t count = 0;
		node* temp = lineStart;
		while (temp != nullptr) {
			count++;
			temp = temp->next;
		}
		return count;
	}
	void insert(char ch) {
		node* new_node = new node(ch);

		if (lines[current_line] == nullptr) {
			lines[current_line] = new_node;
			Cursor = new_node;
		}
		else if (Cursor == nullptr) {
			new_node->next = lines[current_line];
			lines[current_line]->previous = new_node;
			lines[current_line] = new_node;
			Cursor = new_node;
		}
		else {
			new_node->next = Cursor->next;
			new_node->previous = Cursor;
			if (Cursor->next != nullptr) {
				Cursor->next->previous = new_node;
			}
			Cursor->next = new_node;
			Cursor = new_node;
		}
		if (countCharactersInLine(lines[current_line]) > 30) {
			auto tempCursor = Cursor;
			Cursor = nullptr;
			newLine();
			Cursor = lines[current_line];
			while (tempCursor != nullptr && tempCursor->next != nullptr) {
				insert(tempCursor->next->data);
				auto toDelete = tempCursor->next;
				tempCursor->next = tempCursor->next->next;
				if (tempCursor->next) {
					tempCursor->next->previous = tempCursor;
				}
				delete toDelete;
			}
		}
		markModified();
		updateStatus("Insert");
	}


	void moveUp() {
		if (current_line > 0) {
			current_line--;
			Cursor = lines[current_line];
		}
	}

	void moveDown() {
		if (current_line < lines.size() - 1) {
			current_line++;
			Cursor = lines[current_line];
		}
	}

	void moveRight() {
		if (Cursor != nullptr) {
			Iterator it(Cursor);
			++it;
			Cursor = it.getNode();
		}
	}

	void moveLeft() {
		if (Cursor != nullptr) {
			Iterator it(Cursor);
			--it;
			Cursor = it.getNode();
		}
	}

	void newLine() {
		lines.insert(lines.begin() + current_line + 1, nullptr);
		current_line++;
		Cursor = nullptr;
	}

	void enterInsertMode() {
		insertMode = true;
	}

	void exitInsertMode() {
		insertMode = false;
	}

	bool isInsertMode() const {
		return insertMode;
	}
	/*void deleteCurrentLine() {
		if (lines[current_line] == nullptr) {
			return;
		}
		node* temp = lines[current_line];
		while (temp != nullptr) {
			node* to_delete = temp;
			temp = temp->next;
			delete to_delete;
		}
		lines[current_line] = nullptr;
		if (current_line > 0) {
			current_line--;
		}
		else if (current_line < lines.size() - 1) {
			current_line++;
		}

		Cursor = lines[current_line];
	}*/
	void deleteToEndOfLine() {
		if (lines[current_line] == nullptr || Cursor == nullptr) {
			return;
		}
		node* temp = Cursor;
		while (temp != nullptr) {
			node* toDelete = temp;
			temp = temp->next;
			delete toDelete;
		}
		Cursor->next = nullptr;
		Cursor = nullptr;
	}
	void deleteCharacterAtCursor() {
		node* temp = Cursor;

		if (lines[current_line] == nullptr || Cursor == nullptr) {
			return;
		}
		/*if (temp->previous == nullptr && temp->next == nullptr) {
			deleteCurrentLine();
		}*/
		else  if (temp->previous == nullptr) {
			lines[current_line] = temp->next;
			lines[current_line]->previous = nullptr;
			Cursor = lines[current_line];
		}
		else if (temp->next == nullptr) {
			temp->previous->next = nullptr;
			Cursor = temp->previous;
		}
		else {
			temp->previous->next = temp->next;
			temp->next->previous = temp->previous;
			Cursor = temp->next;
		}
		delete temp;
		markModified();
	}
	void backspace() {
		node* temp = Cursor->previous;

		if (lines[current_line] == nullptr || Cursor == nullptr || Cursor->previous == nullptr) {
			return;
		}

		/*if (temp->previous == nullptr) {
			yankLine();
			deleteCurrentLine();
			moveToEndOfLine();
			for (auto ch : copyBuffer) {
				insert(ch);
			}
		}*/
		else {
			temp->previous->next = Cursor;
			Cursor->previous = temp->previous;
		}
		delete temp;
	}
	void yankLine() {
		if (!lines[current_line]) {
			return;
		}
		copyBuffer.clear();
		auto temp = lines[current_line];
		while (temp->next != nullptr) {
			copyBuffer.push_back(temp->data);
			temp = temp->next;
		}
	}
	void pasteAfter() {
		if (copyBuffer.empty()) {
			return;
		}
		newLine();
		for (char ch : copyBuffer) {
			insert(ch);
		}
	}
	void pasteBefore() {
		if (current_line > 0) {
			current_line--;
			Cursor = lines[current_line];
			newLine();
			for (char ch : copyBuffer) {
				insert(ch);
			}
		}
		else {
			Cursor = lines[current_line];
			for (char ch : copyBuffer) {
				insert(ch);
			}
		}
	}
	void moveToStartOfLine() {
		if (!lines[current_line]) {
			return;
		}
		Cursor = lines[current_line];
	}
	void moveToEndOfLine() {
		if (!lines[current_line]) {
			return;
		}
		auto temp = lines[current_line];
		while (temp->next != nullptr) {
			temp = temp->next;
		}
		Cursor = temp;
	}
	void moveToNextWord() {
		node* temp = Cursor;
		if (!temp && current_line + 1 < lines.size()) {
			current_line++;
			Cursor = lines[current_line];
			return;
		}
		while (true) {
			if (!Cursor->next && current_line + 1 < lines.size()) {
				current_line++;
				Cursor = lines[current_line];
				break;
			}
			else if (!Cursor->next && current_line + 1 >= lines.size()) {
				break;
			}
			else {
				while (temp && temp->next && !isWordCharacter(temp->data)) {
					temp = temp->next;
				}
				while (temp && temp->next && isWordCharacter(temp->data)) {
					temp = temp->next;
				}
				while (temp && temp->next && !isWordCharacter(temp->data)) {
					temp = temp->next;
				}
				Cursor = temp;
				break;
			}
		}
	}
	void moveToPreviousWord() {
		if (!Cursor && current_line > 0) {
			current_line--;
			Cursor = lines[current_line];
			moveToEndOfLine();
			return;
		}

		node* temp = Cursor;

		while (true) {
			if (!temp->previous) {
				if (current_line > 0) {
					current_line--;
					Cursor = lines[current_line];
					moveToEndOfLine();
					return;
				}
				return;
			}
			while (temp->previous && (temp->data == ' ' || isPunctuation(temp->data))) {
				temp = temp->previous;
			}
			while (temp->previous && isWordCharacter(temp->data)) {
				temp = temp->previous;
			}
			if (!isWordCharacter(temp->data) && temp->next) {
				temp = temp->next;
			}
			Cursor = temp;
			return;
		}
	}


	void moveToWordEnd() {
		if (!Cursor || !Cursor->next) {
			return;
		}
		auto temp = Cursor;
		while (temp->next && isPunctuation(temp->next->data)) {
			temp = temp->next;
		}
		while (temp->next && isWordCharacter(temp->next->data)) {
			temp = temp->next;
		}
		Cursor = temp;
	}
	void updateStatus(const string& lastCommand = "") {
		status.currentMode = insertMode ? "INSERT" : "NORMAL";
		status.cursorLine = current_line + 1;
		status.cursorColumn = getCursorColumn();
		status.totalLines = lines.size();
		if (!lastCommand.empty()) {
			status.lastCommand = lastCommand;
		}
	}
	size_t getCursorColumn() {
		size_t column = 0;
		auto temp = lines[current_line];
		while (temp && temp != Cursor) {
			column++;
			temp = temp->next;
		}
		return column + 1;
	}


	void display() {
#ifdef _WIN32
		system("cls");
#else
		system("clear");
#endif
		cout << "---------------------------------\n";
		for (int i = 0; i < lines.size(); i++) {
			cout << i + 1 << "|";
			node* temp = lines[i];
			while (temp != nullptr) {
				if (i == current_line && Cursor == temp) {
					cout << "|";
				}
				cout << temp->data;
				temp = temp->next;
			}
			if (i == current_line && Cursor == nullptr) {
				cout << "|";
			}
			cout << endl;
		}
		cout << "---------------------------------\n";

		cout << "Mode: " << status.currentMode
			<< " | File: " << fileManager.getCurrentFileName()
			<< (fileManager.hasUnsavedChanges() ? " [+]" : "")
			<< " | Line: " << status.cursorLine << "/" << status.totalLines
			<< " | Column: " << status.cursorColumn
			<< " | Last: " << status.lastCommand << endl;
	}

	class Iterator {
		node* current;

	public:
		Iterator(node* ptr) : current(ptr) {}

		char operator*() const {
			return current->data;
		}

		Iterator& operator++() {
			if (current != nullptr && current->next != nullptr) {
				current = current->next;
			}
			return *this;
		}

		Iterator& operator--() {
			if (current != nullptr && current->previous != nullptr) {
				current = current->previous;
			}
			return *this;
		}

		bool operator==(const Iterator& other) const {
			return current == other.current;
		}

		bool operator!=(const Iterator& other) const {
			return current != other.current;
		}

		node* getNode() const {
			return current;
		}
	};

};


class CommandMode {

public:    vector<string> commandHistory;    int index = -1;
	  void addCommandToHistory(const string& command) {
		  commandHistory.push_back(command);
		  index = commandHistory.size();
	  }

	  string getPreviousCommand() {
		  if (index > 0) {
			  index--;
			  return commandHistory[index];
		  }
		  return "";
	  }

	  string getNextCommand() {
		  if (index < commandHistory.size() - 1) {
			  index++;
			  return commandHistory[index];
		  }
		  return "";
	  }
};
int getChar() {
#ifdef _WIN32
	int ch = _getch();
	if (ch == 0 || ch == 224) {  // Check for special keys
		ch = _getch();  // Get the second byte
		switch (ch) {
		case 72: return 65;  // Up arrow
		case 80: return 66;  // Down arrow
		case 77: return 67;  // Right arrow
		case 75: return 68;  // Left arrow
		default: return ch + 256;  // Other special keys
		}
	}
	return ch;
#elif defined(linux) || defined(APPLE)
	struct termios old_tio, new_tio;
	int c;

	tcgetattr(STDIN_FILENO, &old_tio);
	new_tio = old_tio;
	new_tio.c_lflag &= (~ICANON & ~ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

	c = getchar();
	if (c == 27) {  // ESC
		c = getchar();
		if (c == 91) {  // [
			c = getchar();
			switch (c) {
			case 65: c = 65; break;  // Up arrow
			case 66: c = 66; break;  // Down arrow
			case 67: c = 67; break;  // Right arrow
			case 68: c = 68; break;  // Left arrow
			default: c += 128;  // Other special keys
			}
		}
		else {
			ungetc(c, stdin);
			c = 27;
		}
	}

	tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
	return c;
#endif
}
// Platform-specific getChar() remains the same
int main() {
	TextEditor editor;
	int command;
	char previousKey = '\0';
	string lastSearchPattern;
	int count = 0; // To handle number prefixes
	CommandMode cmd22;

	while (true) {
		editor.display();
		command = getChar();

		// Check for number prefix
		if (isdigit(command)) {
			count = command - '0'; // Convert char to int
			while (isdigit(command = getChar())) {
				count = count * 10 + (command - '0'); // Build the full number
			}
		}
		else {
			// Handle commands based on the current mode
			if (!editor.isInsertMode()) {
				if (command == ':') {
					string cmd;
					cout << ":";
					cin >> cmd;

					if (cmd == "w") {
						string filename = editor.getFileName();
						if (filename.empty()) {
							cout << "Enter filename to save: ";
							cin >> filename;
						}
						editor.saveToFile(filename);
						cmd22.addCommandToHistory(":w");
					}
					else if (cmd == "wq") {
						string filename = editor.getFileName();
						if (filename.empty()) {
							cout << "Enter filename to save: ";
							cin >> filename;
						}
						editor.saveToFile(filename);
						cmd22.addCommandToHistory(":wq");
						break;
					}
					else if (cmd == "q") {
						if (editor.hasUnsavedChanges()) {
							cout << "Unsaved changes! Use :q! to force quit.\n";
						}
						else {
							break;
						}
					}
					else if (cmd == "q!") {
						cmd22.addCommandToHistory(":q!");
						break;
					}
					else if (cmd == "e") {
						string filename;
						cout << "Enter filename to open: ";
						cin >> filename;
						editor.loadFromFile(filename);
						cmd22.addCommandToHistory(":e " + filename);
					}
					else if (cmd.substr(0, 2) == "s/") {
						// Handle replace commands
						string replaceCmd = cmd.substr(2); // Strip "s/"
						size_t firstSlash = replaceCmd.find('/');
						size_t secondSlash = replaceCmd.rfind('/');

						if (firstSlash != string::npos && secondSlash != string::npos && firstSlash != secondSlash) {
							string oldText = replaceCmd.substr(0, firstSlash);
							string newText = replaceCmd.substr(firstSlash + 1, secondSlash - firstSlash - 1);
							bool replaceAll = (replaceCmd.substr(secondSlash + 1) == "g");

							if (replaceAll) {
								editor.replaceAll(oldText, newText);
							}
							else {
								editor.replaceFirst(oldText, newText);
							}
							cmd22.addCommandToHistory(":s/" + oldText + "/" + newText + (replaceAll ? "/g" : ""));
						}
						else {
							cout << "Invalid replace command. Use :s/old/new or :s/old/new/g.\n";
						}
					}
				}
				else if (command == '/') {
					string pattern;
					cout << "/";
					cin >> pattern;
					lastSearchPattern = pattern;
					editor.search(pattern);
					cmd22.addCommandToHistory("/" + pattern);
				}
				else if (command == 'n') {
					if (!lastSearchPattern.empty()) {
						editor.findNext();
						cmd22.addCommandToHistory("n");
					}
					else {
						cout << "No previous search pattern. Use /pattern first.\n";
					}
				}
				else if (command == 'N') {
					if (!lastSearchPattern.empty()) {
						editor.findPrevious();
						cmd22.addCommandToHistory("N");
					}
					else {
						cout << "No previous search pattern. Use /pattern first.\n";
					}
				}
				else if (command == 'd' || command == 'y' || command == 'j' || command == '>' || command == '<') {
					string cmd;
					switch (command) {
					case 'd':
						cmd = "dd";
						break;
					case 'y':
						cmd = "yy";
						break;
					case 'j':
						cmd = "j";
						break;
					case '>':
						cmd = ">>";
						break;
					case '<':
						cmd = "<<";
						break;
					}
					editor.executeWithCount(count, cmd);
					cmd22.addCommandToHistory(cmd + to_string(count)); // Add command to history with count
					count = 0; // Reset count after execution
				}
			}

			if (command == 27) { // Escape key to exit insert mode
				editor.exitInsertMode();
				previousKey = '\0';
				editor.updateStatus("Exit Insert Mode");
				cmd22.addCommandToHistory("Exit Insert Mode");
				continue;
			}

			auto isArrowKey = [](int key) -> bool {
				return key == 65 || key == 66 || key == 67 || key == 68;
				};

			if (editor.isInsertMode()) {
				if (isArrowKey(command)) {
					switch (command) {
					case 65: editor.moveUp(); break;
					case 66: editor.moveDown(); break;
					case 67: editor.moveRight(); break;
					case 68: editor.moveLeft(); break;
					}
					editor.updateStatus("Arrow Key");
					cmd22.addCommandToHistory("Arrow Key");
				}
				else if (command == 8 || command == 127) { // Handle backspace
					editor.backspace();
					editor.updateStatus("Backspace");
					cmd22.addCommandToHistory("Backspace");
				}
				else {
					editor.insert(static_cast<char>(command));
					cmd22.addCommandToHistory(string(1, static_cast<char>(command)));
				}
			}
			else { // Normal mode
				switch (command) {
				case 'i': // Enter insert mode
					editor.enterInsertMode();
					editor.updateStatus("Enter Insert Mode");
					cmd22.addCommandToHistory("Enter Insert Mode");
					break;
				case 'x':
					editor.deleteCharacterAtCursor();
					editor.updateStatus("Delete Char");
					cmd22.addCommandToHistory("Delete Char");
					break;
				case 'y':
					if (previousKey == 'y') {
						editor.yankLine();
						editor.updateStatus("Yank Line");
						cmd22.addCommandToHistory("Yank Line");
						previousKey = '\0';
					}
					else {
						previousKey = 'y';
					}
					break;
				case 'p':
					editor.pasteAfter();
					editor.updateStatus("Paste After");
					cmd22.addCommandToHistory("Paste After");
					break;
				case 'P':
					editor.pasteBefore();
					editor.updateStatus("Paste Before");
					cmd22.addCommandToHistory("Paste Before");
					break;
				case 'M':
				{
					cout << "Showing command history :" << endl;
					while (true) {
						system("cls");
						cout << "Showing command history:" << endl;

						for (size_t i = 0; i < cmd22.commandHistory.size(); ++i) {
							if (i == cmd22.index) {
								cout << "> " << cmd22.commandHistory[i] << " <" << endl;
							}
							else {
								cout << "  " << cmd22.commandHistory[i] << endl;
							}
						}

						int cmd1 = getChar();

						if (cmd1 == 65) {
							cmd22.getPreviousCommand();
						}
						else if (cmd1 == 66) {
							cmd22.getNextCommand();
						}
						else if (cmd1 == 13) {
							string selectedCommand = cmd22.commandHistory[cmd22.index];
							cout << "Selected command: " << selectedCommand << endl;

							break;
						}
						else if (cmd1 == 27) {
							break;
						}
					}
				}
				break;
				case 'n':
					editor.newLine();
					editor.updateStatus("New Line");
					cmd22.addCommandToHistory("New Line");
					break;
				case '0':
					editor.moveToStartOfLine();
					editor.updateStatus("Move to Start of Line");
					cmd22.addCommandToHistory("Move to Start of Line");
					break;
				case '$':
					editor.moveToEndOfLine();
					editor.updateStatus("Move to End of Line");
					cmd22.addCommandToHistory("Move to End of Line");
					break;
				case 'w':
					editor.moveToNextWord();
					editor.updateStatus("Move to Next Word");
					cmd22.addCommandToHistory("Move to Next Word");
					break;
				case 'b':
					editor.moveToPreviousWord();
					editor.updateStatus("Move to Previous Word");
					cmd22.addCommandToHistory("Move to Previous Word");
					break;
				default:
					if (isArrowKey(command)) {
						switch (command) {
						case 65: editor.moveUp(); break;
						case 66: editor.moveDown(); break;
						case 67: editor.moveRight(); break;
						case 68: editor.moveLeft(); break;
						}
						editor.updateStatus("Arrow Key");
						cmd22.addCommandToHistory("Arrow Key");
					}
					break;
				}
			}
		}
	}

	return 0;
}