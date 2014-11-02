#include "ui.h"
#include <algorithm>
#include <assert.h>

Window::Window(std::unique_ptr<Controller> &&controller):
	_controller(std::move(controller)),
	_framewin(newwin(0, 0, 0, 0)),
	_framepanel(new_panel(_framewin))
{
	draw_chrome();
	_controller->paint(_contentsview);
}

Window::~Window()
{
	del_panel(_framepanel);
	delwin(_framewin);
}

void Window::layout(int xpos, int height, int width, bool lframe, bool rframe)
{
	bool needs_chrome = false;
	int frameheight = height--;
	int framewidth = width;
	if (lframe != _lframe || rframe != _rframe) {
		_lframe = lframe;
		_rframe = rframe;
		needs_chrome = true;
	}
	if (_lframe) {
		xpos -= 1;
		framewidth += 1;
	}
	if (_rframe) {
		framewidth += 1;
	}
	if (frameheight != _height || framewidth != _width) {
		_height = frameheight;
		_width = framewidth;
		_xpos = xpos;
		WINDOW *replacement = newwin(_height, _width, 0, _xpos);
		replace_panel(_framepanel, replacement);
		delwin(_framewin);
		_framewin = replacement;
		needs_chrome = true;
	} else if (xpos != _xpos) {
		_xpos = xpos;
		move_panel(_framepanel, 0, _xpos);
	}
	if (needs_chrome) {
		draw_chrome();
	}
	_contentsview.layout(_framewin, 1, _lframe ? 1 : 0, height, width);
	_controller->paint(_contentsview);
}

void Window::set_focus()
{
	_has_focus = true;
	top_panel(_framepanel);
	draw_chrome();
}

void Window::clear_focus()
{
	_has_focus = false;
	draw_chrome();
}

bool Window::process(int ch)
{
	return _controller->process(_contentsview, ch);
}

void Window::draw_chrome()
{
	// Draw the title bar.
	mvwchgat(_framewin, 0, 0, _width, A_NORMAL, 0, NULL);
	int barx = 0 + (_lframe ? 1 : 0);
	int barwidth = std::max(0, _width + (_lframe ? -1 : 0) + (_rframe ? -1 : 0));
	int titlex = barx + 1;
	int titlewidth = std::max(0, barwidth - 2);
	std::string title = _controller->title();
	if (_has_focus) {
		// Highlight the target with a big reverse-text title.
		title.resize(titlewidth, ' ');
		mvwaddch(_framewin, 0, titlex-1, ' ');
		mvwprintw(_framewin, 0, titlex, title.c_str());
		waddch(_framewin, ' ');
		mvwchgat(_framewin, 0, barx, barwidth, A_REVERSE, 0, NULL);
	} else if (title.size() >= (size_t)titlewidth) {
		// The text will completely fill the space.
		title.resize(titlewidth);
		mvwprintw(_framewin, 0, titlex, title.c_str());
	} else {
		// The text will not completely fill the bar.
		// Draw a continuing horizontal line following.
		mvwprintw(_framewin, 0, titlex, title.c_str());
		int extra = titlewidth - title.size();
		while (extra > 0) {
			mvwaddch(_framewin, 0, barx + barwidth - extra, ACS_HLINE);
			extra--;
		}
	}
	// Draw the left frame, if we have one.
	if (_lframe) {
		mvwaddch(_framewin, 0, 0, ACS_ULCORNER);
		for (int i = 1; i < _height; ++i) {
			mvwaddch(_framewin, i, 0, ACS_VLINE);
		}
	}
	// Draw the right frame, if we have one.
	if (_rframe) {
		int col = _width - 1;
		mvwaddch(_framewin, 0, col, ACS_URCORNER);
		for (int i = 1;  i < _height; ++i) {
			mvwaddch(_framewin, i, col, ACS_VLINE);
		}
	}
}

