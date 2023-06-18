#include "Screen.h"
char const* const FirstScreen::StockMetrics[5] = {
		"1. open",
		"2. high",
		"3. low",
		"4. close",
		"5. volume"
};

Screen::Screen(
	const std::function<void(boost::posix_time::ptime, float)>& populate_Data,
	const std::function<std::vector<std::vector<float> >(uint32_t, uint32_t) >& predict,
	int width, int height
) :
    screenstate{ ScreenState::First },
	m_populate_Data{ populate_Data },
	intelBackgroundImageBuffer{nullptr},
	m_predict{predict},
	m_width{width},m_height{height}
{
	std::filesystem::path stockSymbolFileLocationObject =
		std::filesystem::current_path() / "assets" / "nasdaq_screener_1682959560424.csv";

	std::filesystem::path apiKeyFileLocationObject =
		std::filesystem::current_path() / "assets" / "StocksAPI.key";
    strcpy_s(firstScreen.NameToSymbolCSVFile, stockSymbolFileLocationObject.string().c_str());
    strcpy_s(firstScreen.APIKeyFile, apiKeyFileLocationObject.string().c_str());
	firstScreen.StockMetricSelectionIndex = 0;
}

void Screen::FirstScreenRender() {
	static bool show_demo_window = true;
	ImGui::Begin("API Keys");                          
    /*
	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::InputTextWithHint(
		"Symbol",
		"The location of the CSV data file that contains a column for company names and their stock market symbols", 
		this->firstScreen.NameToSymbolCSVFile, 
		500
	);
	*/
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

	//if (show_demo_window)
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
			{"function" , "TIME_SERIES_INTRADAY"},
			{"interval", "1min" },
			{"symbol" , this->thirdScreen.StockSymbol },
			{"datatype" , "json"},
			{"outputsize" , "compact"}
		},
		cpr::Header{
			{   "X-RapidAPI-Host", "alpha-vantage.p.rapidapi.com"},
			{	"X-RapidAPI-Key" , this->secondScreen.APIKey}
		}
	);

	//auto r = cpr::GetAsync(cpr::Url("https://www.alphavantage.co/query?function=TIME_SERIES_DAILY&symbol=IBM&apikey=demo"));
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
			this->LoadSixthScreen(respose);
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

void Screen::LoadSixthScreen(const cpr::Response& response) {
	const nlohmann::json responseJson = nlohmann::json::parse(response.text);
	const nlohmann::json timeseries = responseJson.at("Time Series (1min)");
	for (auto time_quanta = timeseries.begin(); time_quanta != timeseries.end(); time_quanta++) {
		boost::posix_time::ptime datetime(boost::posix_time::time_from_string(time_quanta.key()));
		const std::string valueString = time_quanta.value().at(this->secondScreen.StockMetricToUse);
		const float value = std::stof(valueString);
		this->m_populate_Data(datetime, value);
		this->internalStockData.insert(std::make_pair(datetime, value));
	}
	generateLargeStockPlot(internalStockData, this->thirdScreen.StockName);
	this->screenstate = ScreenState::Sixth;
}

void Screen::SixthScreenRender() {
	ImGui::Begin("Prediciton parameters");
	/*
	float res = this->m_algorithmCompletionPercent();
	ImGui::Text("Algorithm Completion Percent :: %f %", res);
	if (!this->m_algorithmCompleted()) {
		if (this->m_algorithmIsRunning()) {
			ImGui::Dummy(ImVec2(5.0, 5.0));
			ImSpinner::SpinnerRainbow("#spinner", 40.0f, 1.0f, ImColor(0, 0, 255), 3.0);
			ImGui::Dummy(ImVec2(25.0, 25.0));
			ImGui::Text("Please wait, the algorithm is running");
		}
		else {
			if (ImGui::Button("Next")) {
				this->m_algorithmIterate();
				this->updateAlgorithmProgressPage();
			}
		}
	}
	else {*/
		ImGui::Dummy(ImVec2(15.0, 15.0));
		ImGui::Text("Number of Days to simulate");
		ImGui::InputScalar("p1", ImGuiDataType_U32, &this->sixthScreen.number_of_days_to_simulate);

		ImGui::Dummy(ImVec2(15.0, 15.0));
		ImGui::Text("Number of simulations to run");
		ImGui::InputScalar("p2", ImGuiDataType_U32, &this->sixthScreen.number_of_simulations_to_run);


		ImGui::Dummy(ImVec2(15.0, 15.0));
		if (ImGui::Button("Submit")) {
			this->LoadSixPointFifth();
		}
	//}
		ImGui::End();
}
void Screen::predict() {
	const std::vector<std::vector<float> > prediction= this->m_predict(
		this->sixthScreen.number_of_days_to_simulate,
		this->sixthScreen.number_of_simulations_to_run
		);
	std::vector<float> StocksData;
	int starting_date_offset = max(0, this->internalStockData.size() - 30);
	auto dataIterator = this->internalStockData.begin();
	std::advance(dataIterator, starting_date_offset);
	while (dataIterator != this->internalStockData.end()) {
		StocksData.push_back(dataIterator->second);
		dataIterator++;
	}
	createThePredictionPlot(StocksData, prediction);
}
void Screen::LoadSixPointFifth() {
	this->prediction_async = std::async(
		std::launch::async, 
		&Screen::predict,
		this
		);
	this->screenstate = ScreenState::SixPointFifth;
}
void Screen::SixPointFifthRender() {
	if (this->prediction_async.wait_for(std::chrono::seconds(0)) != std::future_status::ready) {

		ImGui::Begin("Prediciton parameters");
		ImGui::Dummy(ImVec2(5.0, 5.0));
		ImSpinner::SpinnerRainbow("#spinner", 40.0f, 1.0f, ImColor(0, 0, 255), 3.0);
		ImGui::Dummy(ImVec2(25.0, 25.0));
		ImGui::Text("Please wait, the algorithm is running");
		ImGui::End();
	}
	else {
		this->LoadSeventhScreen();
	}
}

void Screen::LoadSeventhScreen() {

	this->screenstate = ScreenState::Seventh;
}


void Screen::SeventhScreenRender() {

}
void Screen::createThePredictionPlot(
	const std::vector<float>& StocksData,
	const std::vector<std::vector<float> >& prediction
) {
	using namespace std;
	RGBABitmapImageReference* canvasReference = CreateRGBABitmapImageReference();
	ScatterPlotSettings* stocksSettings = GetDefaultScatterPlotSettings();
	StringReference* errorMessage = CreateStringReference(toVector(L"Large Stock Plot Generation Failed"));

	int days_before_prediction = StocksData.size();
	int days_being_predicted = prediction[0].size();
	int days = days_before_prediction + days_being_predicted;
	int number_of_simulations = prediction.size();

	std::vector<double>* xs, * ys, *xs_postsim;
	xs = new std::vector<double>(days_before_prediction);
	ys = new std::vector<double>(days_before_prediction);
	xs_postsim = new std::vector<double>(days_being_predicted+1);
	for (int i = 0; i < days_before_prediction; i++) {
		xs->at(i) = i + 1;
	}
	for (int i = 0; i < days_before_prediction; i++) {
		ys->at(i) = StocksData[i];
	}
	for (int stimulation = 0; stimulation <= number_of_simulations; stimulation++) {
		xs_postsim->at(number_of_simulations - stimulation) = days_before_prediction  + stimulation;
	}
	float y_max = *std::max_element(StocksData.begin(), StocksData.end());

	stocksSettings->scatterPlotSeries = new std::vector<ScatterPlotSeries*>(1 + number_of_simulations);
	stocksSettings->scatterPlotSeries->at(0) = new ScatterPlotSeries();
	stocksSettings->scatterPlotSeries->at(0)->xs = xs;
	stocksSettings->scatterPlotSeries->at(0)->ys = ys;
	stocksSettings->scatterPlotSeries->at(0)->linearInterpolation = true;
	stocksSettings->scatterPlotSeries->at(0)->lineType = toVector(L"solid");
	stocksSettings->scatterPlotSeries->at(0)->lineThickness = 3.0;
	stocksSettings->scatterPlotSeries->at(0)->color = CreateRGBColor(0.0, 0.0, 0.0);
	std::random_device rd;
	std::mt19937 e2(rd());
	std::uniform_real_distribution<> dist(0, 1);

	for (int stimulation = 1; stimulation <= number_of_simulations; stimulation++) {
		y_max = max(y_max, *std::max_element(prediction[stimulation-1].begin(), prediction[stimulation - 1].end()));
		stocksSettings->scatterPlotSeries->at(stimulation) = new ScatterPlotSeries();
		stocksSettings->scatterPlotSeries->at(stimulation)->xs = xs_postsim;
		stocksSettings->scatterPlotSeries->at(stimulation)->ys = new std::vector<double>(number_of_simulations);
		stocksSettings->scatterPlotSeries->at(stimulation)->ys->push_back(StocksData.back());
		std::copy(
			prediction[stimulation - 1].rbegin(),
			prediction[stimulation - 1].rend(),
			stocksSettings->scatterPlotSeries->at(stimulation)->ys->begin());
		stocksSettings->scatterPlotSeries->at(stimulation)->linearInterpolation = true;
		stocksSettings->scatterPlotSeries->at(stimulation)->lineType = toVector(L"solid");
		stocksSettings->scatterPlotSeries->at(stimulation)->lineThickness = 2.5;
		float color_r = dist(e2);
		float color_g = dist(e2);
		float color_b = dist(e2);
		stocksSettings->scatterPlotSeries->at(stimulation)->color = CreateRGBColor(color_r, color_g, color_b);
	}

	stocksSettings->autoBoundaries = true;
	stocksSettings->xMin = 0;
	stocksSettings->xMax = days + 3;
	stocksSettings->yMin = 0;
	stocksSettings->yMax = y_max * 1.2;
	stocksSettings->yLabel = toVector(L"Stock Price");
	stocksSettings->xLabel = toVector(L"Time");
	std::wstring widestr = std::wstring(this->thirdScreen.StockName.begin(), this->thirdScreen.StockName.end());
	const wchar_t* widecstr = widestr.c_str();

	stocksSettings->title = toVector(widecstr);
	stocksSettings->width = this->m_width;
	stocksSettings->height = this->m_height;



	bool success = DrawScatterPlotFromSettings(canvasReference, stocksSettings, errorMessage);
	if (!success) {
		throw std::runtime_error("could not draw plot");
	}
	this->largePredictedStocksPlot = new unsigned char[m_width * m_height * 4];
	LoadBitMapIntoOpenGLFormat(this->largePredictedStocksPlot, m_width, m_height, canvasReference);

}
void LoadBitMapIntoOpenGLFormat(
	unsigned char* largeStocksPlot,
	uint32_t m_width, uint32_t m_height,
	RGBABitmapImageReference* canvasReference
) {
	for (uint32_t x = 0; x < m_width; x++) {
		for (uint32_t y = 0; y < m_height; y++) {
			int position = (x + y * m_width) * 4;
			uint32_t x_inverse = x;
			uint32_t y_inverse = m_height - (y + 1);
			largeStocksPlot[position] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->r * 255;
			largeStocksPlot[position + 1] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->g * 255;
			largeStocksPlot[position + 2] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->b * 255;
			largeStocksPlot[position + 3] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->a * 255;
		}
	}
}

void Screen::generateLargeStockPlot(
	const std::map<boost::posix_time::ptime, float>& dateMap,
	const std::string& stockName
) {
	RGBABitmapImageReference* canvasReference;
	canvasReference = CreateRGBABitmapImageReference();
	this->stocksSettings = GetDefaultScatterPlotSettings();
	StringReference* errorMessage = CreateStringReference(toVector(L"Large Stock Plot Generation Failed"));

	int days = min(dateMap.size(), 30);

	std::vector<double>* xs, * ys;
	xs = new std::vector<double>(days);
	ys = new std::vector<double>(days);
	for (int i = 0; i < days; i++) {
		xs->at(i) = i + 1;
		auto iterator = dateMap.rbegin();
		std::advance(iterator, days - (i + 1));
		ys->at(i) = iterator->second;
	}



	this->stocksSettings->scatterPlotSeries = new std::vector<ScatterPlotSeries*>(1.0);
	this->stocksSettings->scatterPlotSeries->at(0) = new ScatterPlotSeries();
	this->stocksSettings->scatterPlotSeries->at(0)->xs = xs;
	this->stocksSettings->scatterPlotSeries->at(0)->ys = ys;
	this->stocksSettings->scatterPlotSeries->at(0)->linearInterpolation = true;
	this->stocksSettings->scatterPlotSeries->at(0)->lineType = toVector(L"solid");
	this->stocksSettings->scatterPlotSeries->at(0)->lineThickness = 3.0;
	this->stocksSettings->scatterPlotSeries->at(0)->color = CreateRGBColor(1.0, 0.0, 0.0);

	this->stocksSettings->autoBoundaries = true;
	this->stocksSettings->xMin = 0;
	this->stocksSettings->xMax = days + 3;
	this->stocksSettings->yMin = 0;
	this->stocksSettings->yMax = (*std::max_element(ys->begin(), ys->end())) * 1.2;
	this->stocksSettings->yLabel = toVector(L"Stock Price");
	this->stocksSettings->xLabel = toVector(L"Time");

	std::wstring widestr = std::wstring(stockName.begin(), stockName.end());
	const wchar_t* widecstr = widestr.c_str();

	this->stocksSettings->title = toVector(widecstr);
	this->stocksSettings->width = this->m_width;
	this->stocksSettings->height = this->m_height;



	bool success = DrawScatterPlotFromSettings(canvasReference, this->stocksSettings, errorMessage);
	if (!success) {
		throw std::runtime_error("could not draw plot");
	}
	this->largeStocksPlot = new unsigned char[m_width * m_height * 4];
	LoadBitMapIntoOpenGLFormat(this->largeStocksPlot, m_width, m_height, canvasReference);

}


unsigned char* Screen::genereateHist(int width, int height, std::vector<float> data, float sum, const wchar_t* title) {
	RGBABitmapImageReference* thetaCanvas = CreateRGBABitmapImageReference();
	BarPlotSettings* thetaSettings = GetDefaultBarPlotSettings();
	thetaSettings->autoBoundaries = true;
	thetaSettings->autoPadding = true;
	thetaSettings->title = toVector(title);
	thetaSettings->showGrid = false;
	thetaSettings->yLabel = toVector(L"Probability");
	thetaSettings->autoColor = true;
	thetaSettings->grayscaleAutoColor = false;
	thetaSettings->autoSpacing = false;
	thetaSettings->autoLabels = true;
	thetaSettings->autoColor = true;

	thetaSettings->width = width;
	thetaSettings->height = height;

	thetaSettings->barBorder = false;
	thetaSettings->barPlotSeries = new std::vector<BarPlotSeries*>(1.0);
	thetaSettings->barPlotSeries->at(0) = GetDefaultBarPlotSeriesSettings();
	thetaSettings->barPlotSeries->at(0)->ys = new std::vector<double>(data.size());
	for (int i = 0; i < data.size(); i++) {
		thetaSettings->barPlotSeries->at(0)->ys->at(i) = data[i] / sum;
	}
	StringReference* errorMessage = CreateStringReference(toVector(L"Large Stock Plot Generation Failed"));
	bool success = DrawBarPlotFromSettings(thetaCanvas, thetaSettings, errorMessage);
	if (!success) {
		throw std::runtime_error("Theta setting not correct");
	}
	unsigned char* result = new unsigned char[width * height * 4];
	LoadBitMapIntoOpenGLFormat(
		result,
		width,
		height,
		thetaCanvas
	);
	return result;
}


void Screen::DrawBackgrounImage() {
	if (this->intelBackgroundImageBuffer == nullptr) {
		std::cout << "nullptr image" << std::endl;
		return;
	}
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, this->intelBackgroundImageBuffer);
}

void Screen::DrawBurnInPleaseWaitImage() {
	if (this->pleaseWaitBurnInImageBuffer == nullptr) {
		std::cout << "nullptr image" << std::endl;
		return;
	}
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, this->pleaseWaitBurnInImageBuffer);
}

void Screen::DrawStockGraph() {
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, this->largeStocksPlot);
}

void Screen::DrawAlgorithm() {
	if (this->algorithm_progress_screen == nullptr) {
		return;
	}
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, this->algorithm_progress_screen);
}
void Screen::DrawPrediction() {
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, this->largePredictedStocksPlot);
}

void Screen::Render() {
	switch (this->screenstate)	{
		case ScreenState::First:
			this->DrawBackgrounImage();
			return this->FirstScreenRender();
		case ScreenState::Second:
			this->DrawBackgrounImage();
			return this->SecondScreenRender();
		case ScreenState::Third:
			this->DrawBackgrounImage();
			return this->ThirdScreenRender();
		case ScreenState::Sixth:
			this->DrawStockGraph();
			return this->SixthScreenRender();
		case ScreenState::SixPointFifth:
			this->DrawStockGraph();
			return this->SixPointFifthRender();
		case ScreenState::Seventh:
			this->DrawPrediction();
			return this->SeventhScreenRender();
		default:
			std::cout << "Default hit in screen render function. Terminating" << std::endl;
			throw std::runtime_error("Default hit in screen render function. Terminating");
	}
}
