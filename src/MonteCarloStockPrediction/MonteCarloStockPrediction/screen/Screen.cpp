#include "Screen.h"

Screen::Screen() :
    screenstate{ ScreenState::First }
{
    strcpy(firstScreen.NameToSymbolCSVFile,"C:\\Users\\raghk\\Documents\\IntelHack\\data\\nasdaq_screener_1682959560424.csv");
    strcpy(firstScreen.APIKeyFile, "C:\\Users\\raghk\\Documents\\IntelHack\\data\\StocksAPI.key.txt");
}

void Screen::FirstScreenRender() {
	static bool show_demo_window = true;
    ImGui::Begin("Select File Location");                          

	ImGui::InputTextWithHint(
		"Symbol",
		"The location of the CSV data file that contains a column for company names and their stock market symbols", 
		this->firstScreen.NameToSymbolCSVFile, 
		500
	);
	ImGui::InputTextWithHint(
		"API Keys",
		"The location of the plain text file that stores the API Keys for Alpha Vantage",
		this->firstScreen.APIKeyFile,
		500
	);

	if (ImGui::Button("Submit"))
		this->LoadSecondScreen();


    ImGui::End();

//	if (show_demo_window)
	//	ImGui::ShowDemoWindow(&show_demo_window);

}

void Screen::LoadSecondScreen() {
	using namespace std;
	ifstream APIfileStream(this->firstScreen.APIKeyFile);
	if (APIfileStream.fail()) {
		throw std::runtime_error(this->firstScreen.APIKeyFile + string(" cannot be opened"));
	}
	getline(APIfileStream, this->secondScreen.APIKey);
	const auto companyNamesFull = ReadCSVExtractStringColumns(
		this->firstScreen.NameToSymbolCSVFile,
		"Name",
		10
	);
	const auto companySybmol = ReadCSVExtractStringColumns(
		this->firstScreen.NameToSymbolCSVFile,
		"Symbol",
		10
	);
	if (companyNamesFull.size() != companySybmol.size()) {
		std::cout << companyNamesFull.size() <<"\t" << companySybmol.size() << std::endl;
		throw std::runtime_error("company name and symbol vectors not of same length");
	}
	
	for (int companyiterator = 0; companyiterator < companySybmol.size(); companyiterator++) {
		this->secondScreen.CompanyNameToSymbol.insert(
			make_pair(
				companyNamesFull.at(companyiterator),
				companySybmol.at(companyiterator)
			)
		);
	}
	this->screenstate = ScreenState::Second;

}

void Screen::SecondScreenRender() {
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_RowBg | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

	ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 20);

	ImGui::Begin("Select Stock to analyse");
	ImGui::BeginTable("Stock Table", 5,flags, outer_size);
	ImGui::TableSetupScrollFreeze(0, 1); // Make top row always visible
	ImGui::TableSetupColumn("One", ImGuiTableColumnFlags_None);
	ImGui::TableSetupColumn("Two", ImGuiTableColumnFlags_None);
	ImGui::TableSetupColumn("Three", ImGuiTableColumnFlags_None);
	ImGui::TableSetupColumn("Four", ImGuiTableColumnFlags_None);
	ImGui::TableSetupColumn("Five", ImGuiTableColumnFlags_None);
	ImGui::TableHeadersRow();

	// Demonstrate using clipper for large vertical lists
	ImGuiListClipper clipper;
	clipper.Begin(this->secondScreen.CompanyNameToSymbol.size() / 5);
	std::map<std::string, std::string>::iterator 
		CompanyNameToSymbolIterator = this->secondScreen.CompanyNameToSymbol.begin();
	while (clipper.Step())
	{
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
		{
			ImGui::TableNextRow();
			for (int column = 0; column < 5; column++)
			{
				ImGui::TableSetColumnIndex(column);
				ImGui::Text(CompanyNameToSymbolIterator->first.data());
				std::advance(CompanyNameToSymbolIterator, 1);
			}
		}
	}
	ImGui::EndTable();
	ImGui::End();
}
void Screen::Render() {
	switch (this->screenstate)
	{
	case ScreenState::First:
		return this->FirstScreenRender();
	case ScreenState::Second:
		return this->SecondScreenRender();
	default:
		std::cout << "Default hit in screen render function. Terminating" << std::endl;
		std::terminate();
	}
}