#include <gui.h>
#include <steam.h>


int main(int argc, char* argv[])
{
    Gui::initialise();

    Steam::setSteamCmdOp(Steam::SteamCmdUpdate);

    while(!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        Steam::handle();
        Gui::handle();

        EndDrawing();
    }

    Gui::shutdownGui();
    Gui::shutdownServer();

    CloseWindow();

    return 0;
}
