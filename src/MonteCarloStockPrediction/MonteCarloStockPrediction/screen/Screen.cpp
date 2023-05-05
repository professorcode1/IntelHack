#include "Screen.h"

Screen::Screen() :
    screenstate{ ScreenState::First }
{
    strcpy_s(firstScreen.NameToSymbolCSVFile,"C:\\Users\\raghk\\Documents\\IntelHack\\data\\nasdaq_screener_1682959560424.csv");
    strcpy_s(firstScreen.APIKeyFile, "C:\\Users\\raghk\\Documents\\IntelHack\\data\\StocksAPI.key.txt");
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

	if (show_demo_window)
		ImGui::ShowDemoWindow(&show_demo_window);

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
		"Name"
	);
	const auto companySybmol = ReadCSVExtractStringColumns(
		this->firstScreen.NameToSymbolCSVFile,
		"Symbol"
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
//	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();

//	ImVec2 outer_size = ImVec2(0.0f, TEXT_BASE_HEIGHT * 20);
	constexpr int columnCount = 5;
	ImGui::Begin("Select Stock to analyse");
	if (!ImGui::BeginTable("Stock Table", columnCount, flags)) {
		return;
	}

	std::map<std::string, std::string>::iterator StockSelected;
	bool isStockSelected = false;
	const int rowCount = this->secondScreen.CompanyNameToSymbol.size() / columnCount + 
		(this->secondScreen.CompanyNameToSymbol.size() % 5 > 0);
	ImGuiListClipper clipper;
	clipper.Begin(rowCount);
	while (clipper.Step())
	{
		std::map<std::string, std::string>::iterator
			CompanyNameToSymbolIterator = this->secondScreen.CompanyNameToSymbol.begin();
		const int advance = min(clipper.DisplayStart * columnCount, (int)this->secondScreen.CompanyNameToSymbol.size());
		std::advance(CompanyNameToSymbolIterator, advance);
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
		{
			ImGui::TableNextRow();
			for (int column = 0; column < 5; column++)
			{
				ImGui::TableSetColumnIndex(column);
				if (CompanyNameToSymbolIterator != this->secondScreen.CompanyNameToSymbol.end()) {
					if (ImGui::Button(CompanyNameToSymbolIterator->first.data())) {
						isStockSelected = true;
						StockSelected = CompanyNameToSymbolIterator;
					};
					std::advance(CompanyNameToSymbolIterator, 1);
				}
				else {
					ImGui::Text("");
				}
			}
		}
	}
	ImGui::EndTable();
	ImGui::End();
	if (isStockSelected) {
		this->LoadThirdScreen(StockSelected);
	}
}
void Screen::LoadThirdScreen(std::map<std::string, std::string>::iterator StockSelected) {
	this->thirdScreen.StockName = StockSelected->first;
	this->thirdScreen.StockSymbol = StockSelected->second;
	
	auto r = cpr::GetAsync(
		cpr::Url("https://alpha-vantage.p.rapidapi.com/query"),
		cpr::Parameters{
			{"function" , "TIME_SERIES_DAILY"},
			{"symbol" , this->thirdScreen.StockSymbol },
			{"datatype" , "json"},
			{"outputsize" , "compact"}
		},
		cpr::Header{
			{   "X-RapidAPI-Host", "alpha-vantage.p.rapidapi.com"},
			{	"X-RapidAPI-Key" , this->secondScreen.APIKey}
		}
	);
	this->thirdScreen.APICallResponse = new cpr::AsyncResponse(std::move(r));
	this->screenstate = ScreenState::Third;
}

void Screen::ThirdScreenRender() {
	if (this->thirdScreen.APICallResponse->wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
		std::cout << this->thirdScreen.APICallResponse->get().text << std::endl;
		this->screenstate = ScreenState::Fourth;
	}
	else {
		ImGui::Begin("Stock data is being downloaded");
		ImGui::Text("Please wait");
		ImGui::End();
	}
}

void Screen::Render() {
	switch (this->screenstate)
	{
	case ScreenState::First:
		return this->FirstScreenRender();
	case ScreenState::Second:
		return this->SecondScreenRender();
	case ScreenState::Third:
		return this->ThirdScreenRender();
	default:
		std::cout << "Default hit in screen render function. Terminating" << std::endl;
		std::terminate();
	}
}
