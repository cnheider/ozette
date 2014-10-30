#include "ui.h"

int main()
{
	UI ui;
        // Build our two starting windows.
        std::unique_ptr<Window::Controller> console(new Console);
        ui.open(std::move(console));
        std::unique_ptr<Window::Controller> browser(new Browser);
        ui.open(std::move(browser));
	do {
		update_panels();
		doupdate();
	} while (ui.process(getch()));
	return 0;
}
