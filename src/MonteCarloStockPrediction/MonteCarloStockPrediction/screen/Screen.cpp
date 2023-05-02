#include "Screen.h"

Screen::Screen() :
    screenstate{ ScreenState::First }
{
    strcpy(firstScreen.NameToSymbolCSVFile,"C:\\Users\\raghk\\Documents\\IntelHack\\data\\nasdaq_screener_1682959560424.csv");
    strcpy(firstScreen.APIKeyFile, "C:\\Users\\raghk\\Documents\\IntelHack\\data\\StocksAPI.key.txt");
}

void Screen::FirstScreenRender() {

    ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

	ImGui::InputText("string1", this->firstScreen.NameToSymbolCSVFile, 500);
	ImGui::InputText("string2", this->firstScreen.APIKeyFile, 500);


    ImGui::End();

}

void Screen::Render() {
	switch (this->screenstate)
	{
	case ScreenState::First:
		return this->FirstScreenRender();
	default:
		std::cout << "Default hit in screen render function. Terminating" << std::endl;
		std::terminate();
	}
}