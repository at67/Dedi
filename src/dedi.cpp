#include <gui.h>
#include <steam.h>


int main(int argc, char* argv[])
{
    Gui::initialise();

    Steam::setCmdOp(Steam::CmdInit);

    while(!WindowShouldClose())
    {
        BeginDrawing();

        ClearBackground(GetColor(GuiGetStyle(DEFAULT, BACKGROUND_COLOR)));

        Steam::handle();

        Gui::handle();

        EndDrawing();
    }

    CloseWindow();

    return 0;
}
