#include <win.h>
#include <gui.h>
#include <steam.h>


int main(int argc, char* argv[])
{
    Gui::initialise();

    Steam::setCmdOp(Steam::CmdUpdate);

    while(!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        Win::readConsoleText();
        Steam::handle();
        Gui::handle();
        Win::clearConsoleText();

        EndDrawing();
    }

    Gui::shutdownServer();

    CloseWindow();

    return 0;
}
