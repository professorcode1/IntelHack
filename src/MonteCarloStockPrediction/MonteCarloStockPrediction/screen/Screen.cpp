#include "Screen.h"
char const* const FirstScreen::StockMetrics[5] = {
		"1. open",
		"2. high",
		"3. low",
		"4. close",
		"5. volume"
};

Screen::Screen() :
    screenstate{ ScreenState::First }
{
    strcpy_s(firstScreen.NameToSymbolCSVFile,"C:\\Users\\raghk\\Documents\\IntelHack\\data\\nasdaq_screener_1682959560424.csv");
    strcpy_s(firstScreen.APIKeyFile, "C:\\Users\\raghk\\Documents\\IntelHack\\data\\StocksAPI.key");
	firstScreen.StockMetricSelectionIndex = 0;
}

void Screen::FirstScreenRender() {
	static bool show_demo_window = true;
    ImGui::Begin("Select File Location");                          
	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::InputTextWithHint(
		"Symbol",
		"The location of the CSV data file that contains a column for company names and their stock market symbols", 
		this->firstScreen.NameToSymbolCSVFile, 
		500
	);
	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::InputTextWithHint(
		"API Keys",
		"The location of the plain text file that stores the API Keys for Alpha Vantage",
		this->firstScreen.APIKeyFile,
		500
	);
	ImGui::Dummy(ImVec2(15.0, 15.0));
	int &stockIndex = firstScreen.StockMetricSelectionIndex;
	const char* combo_preview_value = FirstScreen::StockMetrics[stockIndex];
	if (ImGui::BeginCombo("Select Stock Metric", combo_preview_value, 0))
	{
		for (int n = 0; n < 5; n++)
		{
			const bool is_selected = (firstScreen.StockMetricSelectionIndex == n);
			if (ImGui::Selectable(FirstScreen::StockMetrics[n], is_selected))
				stockIndex = n;

			// Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
			if (is_selected)
				ImGui::SetItemDefaultFocus();
		}
		ImGui::EndCombo();
	}

	ImGui::Dummy(ImVec2(15.0, 15.0));

	if (ImGui::Button("Submit"))
		this->LoadSecondScreen();
	ImGui::Dummy(ImVec2(5.0, 5.0));


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
	this->secondScreen.StockMetricToUse = FirstScreen::StockMetrics[firstScreen.StockMetricSelectionIndex];
	this->screenstate = ScreenState::Second;

}

void Screen::SecondScreenRender() {
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY |  ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;
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
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
	const float min_row_height = (float)(int)(TEXT_BASE_HEIGHT * 0.30f * 5);

	while (clipper.Step())
	{
		std::map<std::string, std::string>::iterator
			CompanyNameToSymbolIterator = this->secondScreen.CompanyNameToSymbol.begin();
		const int advance = min(clipper.DisplayStart * columnCount, (int)this->secondScreen.CompanyNameToSymbol.size());
		std::advance(CompanyNameToSymbolIterator, advance);
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++)
		{
			ImGui::TableNextRow(ImGuiTableRowFlags_None, min_row_height);
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
		const cpr::Response respose = this->thirdScreen.APICallResponse->get();
		if (respose.error) {
			ImGui::Begin("Error");
			ImGui::Dummy(ImVec2(5.0, 5.0));
			ImGui::Text("Code :: %d", respose.error.code);
			ImGui::Text(respose.error.message.c_str());
			ImGui::End();
		}
		else {
			this->LoadFourthScreen(respose);
		}
	}
	else {
		ImGui::Begin("Downloading");
		ImGui::Dummy(ImVec2(5.0, 5.0));
		ImSpinner::SpinnerRainbow("#spinner", 40.0f, 1.0f, ImColor(0, 0, 255), 3.0);
		ImGui::Dummy(ImVec2(25.0, 25.0));
		ImGui::Text("Please wait, the data is being downloaded");
		ImGui::End();
	}
}

void Screen::LoadFourthScreen(const cpr::Response& response) {
	using namespace boost::gregorian;
		const nlohmann::json responseJson = nlohmann::json::parse(response.text);
		const nlohmann::json timeseries = responseJson.at("Time Series (Daily)");
		for (auto time_quanta = timeseries.begin(); time_quanta != timeseries.end(); time_quanta++) {
			date date(from_simple_string(time_quanta.key()));
			const std::string valueString = time_quanta.value().at(this->secondScreen.StockMetricToUse);
			const float value = std::stof(valueString);
			this->fourthScreen.data.insert(std::make_pair(date, value));
		}
	this->screenstate = ScreenState::Fourth;
}

void Screen::FourthScreenRender() {
	static ImGuiTableFlags flags = ImGuiTableFlags_ScrollY | ImGuiTableFlags_BordersOuter | ImGuiTableFlags_BordersV | ImGuiTableFlags_Resizable | ImGuiTableFlags_Reorderable | ImGuiTableFlags_Hideable;

	ImGui::Begin("Select Stock to analyse");
	if (!ImGui::BeginTable("Stock Table", 1, flags)) {
		return;
	}

	int rowCount = this->fourthScreen.data.size();
	ImGuiListClipper clipper;
	clipper.Begin(rowCount);
	const float TEXT_BASE_HEIGHT = ImGui::GetTextLineHeightWithSpacing();
	const float min_row_height = (float)(int)(TEXT_BASE_HEIGHT * 0.30f * 5);

	while (clipper.Step()) {
		std::map<boost::gregorian::date, float>::iterator
			CompanyNameToSymbolIterator = this->fourthScreen.data.begin();
		const int advance_left = clipper.DisplayStart;
		std::advance(CompanyNameToSymbolIterator, advance_left);
		for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
			ImGui::TableNextRow(ImGuiTableRowFlags_None, min_row_height);
			ImGui::TableSetColumnIndex(0);
			std::string text = to_simple_string(CompanyNameToSymbolIterator->first);
			text += " :: %f";
			ImGui::Text(text.c_str(), CompanyNameToSymbolIterator->second);
			std::advance(CompanyNameToSymbolIterator, 1);
		}
	}
	ImGui::EndTable();
	ImGui::End();
}

void Screen::Render() {
	switch (this->screenstate)	{
	case ScreenState::First:
		return this->FirstScreenRender();
	case ScreenState::Second:
		return this->SecondScreenRender();
	case ScreenState::Third:
		return this->ThirdScreenRender();
	case ScreenState::Fourth:
		return this->FourthScreenRender();
	default:
		std::cout << "Default hit in screen render function. Terminating" << std::endl;
		std::terminate();
	}
}
