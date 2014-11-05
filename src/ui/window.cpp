#include "shell.h"
#include <algorithm>
#include <assert.h>

UI::Window::Window(App &app, std::unique_ptr<Controller> &&controller):
	_app(app),
	_controller(std::move(controller)),
	_framewin(newwin(0, 0, 0, 0)),
	_framepanel(new_panel(_framewin)),
	_contentwin(newwin(0, 0, 0, 0)),
	_contentpanel(new_panel(_contentwin))
{
	_controller->open(*this);
	paint();
}

UI::Window::~Window()
{
	del_panel(_framepanel);
	delwin(_framewin);
	del_panel(_contentpanel);
	delwin(_contentwin);
}

void UI::Window::layout(int xpos, int height, int width, bool lframe, bool rframe)
{
	// Windows are vertical slices of screen space.
	// Given this column number and a width, compute the
	// window's nominal dimensions, then see if we need to adjust
	// the actual window to match.
	int screen_height, screen_width;
	getmaxyx(stdscr, screen_height, screen_width);
	// We will expand our edges by one pixel in each direction if
	// we have space for it so that we can draw a window frame.
	_lframe = xpos > 0;
	if (_lframe) {
		xpos--;
		width++;
	}
	_rframe = (xpos + width) < screen_width;
	if (_rframe) {
		width++;
	}

	// What are the dimensions and location of our existing frame?
	// If the window has the wrong size, we must rebuild it, and
	// then we should layout our content window too. Otherwise, if
	// the window is in the wrong place, simply relocate it.
	int old_height, old_width;
	getmaxyx(_framewin, old_height, old_width);
	int old_vert, old_horz;
	getbegyx(_framewin, old_vert, old_horz);
	if (old_height != screen_height || old_width != width) {
		WINDOW *replacement = newwin(height, width, 0, xpos);
		replace_panel(_framepanel, replacement);
		delwin(_framewin);
		_framewin = replacement;
		_dirty_chrome = true;
	} else if (old_vert != 0 || old_horz != xpos) {
		move_panel(_framepanel, 0, xpos);
	}
	layout_contentwin();
	paint();
}

void UI::Window::set_focus()
{
	_has_focus = true;
	_dirty_chrome = true;
	_dirty_content = true;
	paint();
}

void UI::Window::clear_focus()
{
	_has_focus = false;
	_dirty_chrome = true;
	_dirty_content = true;
	paint();
}

void UI::Window::bring_forward()
{
	top_panel(_framepanel);
	top_panel(_contentpanel);
}

bool UI::Window::process(int ch)
{
	bool out = _controller->process(*this, ch);
	paint();
	return out;
}

bool UI::Window::poll()
{
	return process(ERR);
}

void UI::Window::set_title(std::string text)
{
	_title = text;
	_dirty_chrome = true;
}

void UI::Window::set_status(std::string text)
{
	_status = text;
	_dirty_chrome = true;
}

void UI::Window::set_help(const help_panel_t *help)
{
	_help = help;
	_dirty_chrome = true;
}

void UI::Window::layout_contentwin()
{
	// Compute the location and dimension of the content window,
	// relative to the location and dimension of the frame window.
	int new_height, new_width;
	getmaxyx(_framewin, new_height, new_width);
	int new_vpos, new_hpos;
	getbegyx(_framewin, new_vpos, new_hpos);

	// Adjust these dimensions inward to account for the space
	// used by the frame. Every window has a title bar.
	new_vpos++;
	new_height--;
	// The window may have a one-column left frame.
	if (_lframe) {
		new_hpos++;
		new_width--;
	}
	// The window may have a one-column right frame.
	if (_rframe) {
		new_width--;
	}
	// There may be a task bar, whose height may vary.
	new_height -= _taskbar_height;

	// Find out how where and how large the content window is.
	// If our existing content window is the wrong size, recreate it.
	// Otherwise, if it needs to be relocated, move its panel.
	int old_height, old_width;
	getmaxyx(_contentwin, old_height, old_width);
	int old_vpos, old_hpos;
	getbegyx(_contentwin, old_vpos, old_hpos);
	if (old_height != new_height || old_width != new_width) {
		WINDOW *win = newwin(new_height, new_width, new_vpos, new_hpos);
		replace_panel(_contentpanel, win);
		delwin(_contentwin);
		_contentwin = win;
		_dirty_content = true;
	} else if (old_vpos != new_vpos || old_hpos != new_vpos) {
		move_panel(_contentpanel, new_vpos, new_hpos);
	}
}

void UI::Window::paint()
{
	if (_dirty_content) paint_content();
	if (_dirty_chrome) paint_chrome();
}

void UI::Window::paint_content()
{
	_controller->paint(_contentwin, _has_focus);
	_dirty_content = false;
}

void UI::Window::paint_chrome()
{
	int height, width;
	getmaxyx(_framewin, height, width);
	paint_title_bar(height, width);
	if (_lframe) paint_left_frame(height, width);
	if (_rframe) paint_right_frame(height, width);
	if (_taskbar_height) paint_task_bar(height, width);
	_dirty_chrome = false;
}

void UI::Window::paint_title_bar(int height, int width)
{
	// Draw corners and a horizontal line across the top.
	wmove(_framewin, 0, 0);
	if (_lframe) waddch(_framewin, ACS_ULCORNER);
	for (int i = 0; i+1 < width; ++i) {
		waddch(_framewin, ACS_HLINE);
	}
	if (_rframe) waddch(_framewin, ACS_URCORNER);

	// Overwrite the bar line with the window title.
	int left = _lframe ? 3 : 2;
	int right = width - (_rframe ? 3 : 2);
	width = right - left;
	int titlechars = width - 2;
	mvwaddch(_framewin, 0, left, ' ');
	waddnstr(_framewin, _title.c_str(), titlechars);
	waddch(_framewin, ' ');

	// If there is a status string, print it on the right side.
	if (_status.empty()) return;
	int statchars = std::min((int)_status.size(), titlechars/2);
	mvwaddch(_framewin, 0, right - statchars - 2, ' ');
	waddnstr(_framewin, _status.c_str(), statchars);
	waddch(_framewin, ' ');
}

void UI::Window::paint_left_frame(int height, int width)
{
	int maxv = height - _taskbar_height;
	for (int i = 1; i < maxv; ++i) {
		mvwaddch(_framewin, i, 0, ACS_VLINE);
	}
}

void UI::Window::paint_right_frame(int height, int width)
{
	int maxv = height - _taskbar_height;
	int maxh = width - 1;
	for (int i = 1;  i < maxv; ++i) {
		mvwaddch(_framewin, i, maxh, ACS_VLINE);
	}
}

void UI::Window::paint_task_bar(int height, int width)
{
	int loweredge = height - _taskbar_height;
	mvwaddch(_framewin, loweredge, 0, ACS_LLCORNER);
	for (int i = 0; i < width-1; ++i) {
		waddch(_framewin, ACS_HLINE);
	}
	waddch(_framewin, ACS_LRCORNER);
}
