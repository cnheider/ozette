#ifndef EDITOR_DOCUMENT_H
#define EDITOR_DOCUMENT_H

#include <string>
#include <vector>

// A document maps a text buffer into an infinite plane
// of equally-sized character cells. It is responsible for
// interpreting the formatting control characters.
namespace Editor {
class Document
{
public:
	typedef size_t line_t;
	typedef size_t offset_t;
	typedef unsigned row_t;
	typedef unsigned column_t;

	Document(std::string targetpath);
	static const unsigned kTabWidth;
	line_t maxline() const { return _maxline; }
	std::string get_line_text(line_t index) const;
	size_t get_line_size(line_t index) const;
	offset_t char_for_column(column_t h, line_t index) const;
	column_t column_for_char(offset_t c, line_t index) const;
	unsigned char_width(char ch, column_t h) const;
	unsigned tab_width(column_t h) const;
private:
	std::vector<std::string> _lines;
	line_t _maxline = 0;	// ubound, not size
};
} // namespace Editor

#endif // EDITOR_DOCUMENT_H
