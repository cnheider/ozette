#ifndef BROWSER_DIRTREE_H
#define BROWSER_DIRTREE_H

#include <vector>
#include <string>

class DirTree
{
public:
	DirTree(std::string path);
	DirTree(std::string dir, std::string name);
	void scan();
	std::string path() const { return _path; }
	std::string name() const { return _name; }
	enum class Type {
		Directory,
		File,
		Other,
		None
	};
	bool is_directory() { return type() == Type::Directory; }
	Type type() { initcheck(); return _type; }
	time_t mtime() { initcheck(); return _mtime; }
	std::vector<DirTree> &items();
private:
	void initcheck() { if (!_scanned) scan(); }
	void iterate();
	std::string _path;
	std::string _name;
	bool _scanned = false;
	Type _type = Type::None;
	time_t _mtime = 0;
	bool _iterated = false;
	std::vector<DirTree> _items;
};

#endif	//BROWSER_DIRTREE_H
