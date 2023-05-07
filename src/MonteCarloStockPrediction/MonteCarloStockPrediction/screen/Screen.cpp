#include "Screen.h"
char const* const FirstScreen::StockMetrics[5] = {
		"1. open",
		"2. high",
		"3. low",
		"4. close",
		"5. volume"
};

Screen::Screen(
	const std::function<void(boost::gregorian::date, float)>& populate_Data,
	const std::vector<cl::sycl::device>& AllDevice,
	const std::function<void(std::string, int)>& populate_DeviceWorkloadPreference,
	const std::function<AlgorithmParameter& ()>& parameterReference,
	const std::function<void()> initialiseAlgorithm,
	const std::function<void()>& algorithmIterate,
	const std::function<float()>& algorithmCompletionPercent,
	const std::function<bool()>& algorithmCompleted,
	const std::function<AlgorithmResponse()>& algorithmResonse,
	int width, int height
) :
    screenstate{ ScreenState::First },
	m_populate_Data{ populate_Data },
	m_AllDevice{AllDevice},
	m_populate_DeviceWorkloadPreference{ populate_DeviceWorkloadPreference },
	m_parameterReference{ parameterReference },
	m_initialiseAlgorithm{ initialiseAlgorithm },
	intelBackgroundImageBuffer{nullptr},
	m_algorithmIterate{ algorithmIterate },
	m_algorithmCompletionPercent{ algorithmCompletionPercent },
	m_algorithmCompleted{ algorithmCompleted },
	m_algorithmResonse{ algorithmResonse },
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
	std::map<date, float> internalStockData;
	for (auto time_quanta = timeseries.begin(); time_quanta != timeseries.end(); time_quanta++) {
		date date(from_simple_string(time_quanta.key()));
		const std::string valueString = time_quanta.value().at(this->secondScreen.StockMetricToUse);
		const float value = std::stof(valueString);
		this->m_populate_Data(date, value);
		internalStockData.insert(std::make_pair(date, value));
	}
	this->fourthScreen.preference.resize(this->m_AllDevice.size());
	generateLargeStockPlot(internalStockData, this->thirdScreen.StockName);
	std::fill(this->fourthScreen.preference.begin(), this->fourthScreen.preference.end(), 0);
	this->screenstate = ScreenState::Fourth;
}

void Screen::FourthScreenRender() {
	ImGui::Begin("Device Workload");
	const int deviceCount = this->m_AllDevice.size();
	ImGui::Dummy(ImVec2(15.0, 15.0));
	std::set<std::string> printed;
	for (int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		std::string DeviceName = this->m_AllDevice[deviceIndex].get_info<cl::sycl::info::device::name>();
		if (printed.find(DeviceName) != printed.end()) {
			continue;
		}
		printed.insert(DeviceName);
		ImGui::Text(DeviceName.c_str());
		const int max_compute_units = this->m_AllDevice[deviceIndex].get_info<cl::sycl::info::device::max_compute_units>();
		ImGui::Text("Compute Units :: %d", max_compute_units);
		const std::string sliderName = std::string("Device #") + std::to_string(deviceIndex);
		ImGui::SliderInt(sliderName.c_str(), this->fourthScreen.preference.data() + deviceIndex, 0, 100);
		ImGui::Dummy(ImVec2(15.0, 15.0));
	}
	if (ImGui::Button("Submit")) {
		this->LoadFifthScreen();
	}

	ImGui::End();
}

void Screen::LoadFifthScreen() {
	const int deviceCount = this->m_AllDevice.size();
	for (int deviceIndex = 0; deviceIndex < deviceCount; deviceIndex++) {
		this->m_populate_DeviceWorkloadPreference(
			this->m_AllDevice[deviceIndex].get_info<cl::sycl::info::device::name>(),
			this->fourthScreen.preference[deviceIndex]
		);
	}
	this->screenstate = ScreenState::Fifth;
}
void Screen::FifthScreenRender() {
	static bool inputs_step = true;
	const float f32_one = 1.0f;
	AlgorithmParameter& parameter = this->m_parameterReference();
	ImGui::Begin("Fifth Screen");

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Volatility Theta Parameters");
	ImGui::InputScalar("lower #1", ImGuiDataType_Float, &parameter.m_volatility_theta.lower);
	ImGui::InputScalar("upper #1", ImGuiDataType_Float, &parameter.m_volatility_theta.upper);
	ImGui::InputScalar("test value #1", ImGuiDataType_Float, &parameter.m_volatility_theta.testval);
	ImGui::InputScalar("guassian step standard deviation #1", ImGuiDataType_Float, &parameter.m_volatility_theta.guassian_step_sd);
	

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Volatility Mu Parameters");
	ImGui::InputScalar("mean", ImGuiDataType_Float, &parameter.m_volatility_mu.mean);
	ImGui::InputScalar("standard deviation", ImGuiDataType_Float, &parameter.m_volatility_mu.sd);
	ImGui::InputScalar("test value #2", ImGuiDataType_Float, &parameter.m_volatility_mu.testval);
	ImGui::InputScalar("guassian step standard deviation #2", ImGuiDataType_Float, &parameter.m_volatility_mu.guassian_step_sd);
	ImGui::InputScalar("Buffer range sigma multiplier", ImGuiDataType_U32, &parameter.m_volatility_mu.buffer_range_sigma_multiplier);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Volatility Sigma Parameters");
	ImGui::InputScalar("lower #2", ImGuiDataType_Float, &parameter.m_volatility_sigma.lower);
	ImGui::InputScalar("upper #2", ImGuiDataType_Float, &parameter.m_volatility_sigma.upper);
	ImGui::InputScalar("test value #3", ImGuiDataType_Float, &parameter.m_volatility_sigma.testval);
	ImGui::InputScalar("guassian step standard deviation #3", ImGuiDataType_Float, &parameter.m_volatility_sigma.guassian_step_sd);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Volatility Parameters");
	ImGui::InputScalar("time delta", ImGuiDataType_Float, &parameter.m_volatility.dt);
	ImGui::InputScalar("brownian Motion delta lower bound", ImGuiDataType_Float, &parameter.m_volatility.dw_lower);
	ImGui::InputScalar("brownian Motion delta upper bound", ImGuiDataType_Float, &parameter.m_volatility.dw_upper);
	ImGui::InputScalar("test value #4", ImGuiDataType_Float, &parameter.m_volatility.testval);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Number of iterations");
	ImGui::InputScalar("p1", ImGuiDataType_U32, &parameter.m_MCMCIteration);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Number of iterations after which graphs update");
	ImGui::InputScalar("p2", ImGuiDataType_U32, &parameter.m_GraphUpdateIteration);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Number of days to use for stock analysis");
	ImGui::InputScalar("p3", ImGuiDataType_U32, &parameter.m_NumberOfDaysToUse);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Burn In");
	ImGui::InputScalar("p4", ImGuiDataType_U32, &parameter.m_BurnIn);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("Number of discrete intervals to break down continuous spaces");
	ImGui::InputScalar("p5", ImGuiDataType_U32, &parameter.m_DiscretCountOfContinuiosSpace);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("HMC Leapfrog");
	ImGui::InputScalar("p6", ImGuiDataType_U32, &parameter.m_leapfrog);

	ImGui::Dummy(ImVec2(15.0, 15.0));
	ImGui::Text("HMC Epsilon");
	ImGui::InputScalar("p7", ImGuiDataType_Float, &parameter.m_epsilon);


	ImGui::Dummy(ImVec2(15.0, 15.0));
	if (ImGui::Button("Submit")) {
		this->LoadSixthScreen();
	}
	
	ImGui::End();
}

void Screen::LoadSixthScreen() {
	this->m_initialiseAlgorithm();
	this->screenstate = ScreenState::Sixth;
}

void Screen::SixthScreenRender() {
	ImGui::Begin("6th screen");
	float res = this->m_algorithmCompletionPercent();
	std::cout << res << std::endl;
	ImGui::Text("Algorithm Completion Percnet :: %f %", res);
	ImGui::End();
	if (this->m_algorithmCompleted()) {
		this->screenstate = ScreenState::Seventh;
	}
	this->m_algorithmIterate();
}

void Screen::generateLargeStockPlot(
	const std::map<boost::gregorian::date, float>& dateMap,
	const std::string& stockName
) {
	RGBABitmapImageReference* canvasReference;
	canvasReference = CreateRGBABitmapImageReference();
	ScatterPlotSettings* settings;
	settings = GetDefaultScatterPlotSettings();
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



	settings->scatterPlotSeries = new std::vector<ScatterPlotSeries*>(1.0);
	settings->scatterPlotSeries->at(0) = new ScatterPlotSeries();
	settings->scatterPlotSeries->at(0)->xs = xs;
	settings->scatterPlotSeries->at(0)->ys = ys;
	settings->scatterPlotSeries->at(0)->linearInterpolation = true;
	settings->scatterPlotSeries->at(0)->lineType = toVector(L"solid");
	settings->scatterPlotSeries->at(0)->lineThickness = 3.0;
	settings->scatterPlotSeries->at(0)->color = CreateRGBColor(1.0, 0.0, 0.0);

	settings->autoBoundaries = false;
	settings->xMin = 0;
	settings->xMax = days + 3;
	settings->yMin = 0;
	settings->yMax = (*std::max_element(ys->begin(), ys->end())) * 1.2;
	settings->yLabel = toVector(L"Stock Price");
	settings->xLabel = toVector(L"Time");

	std::wstring widestr = std::wstring(stockName.begin(), stockName.end());
	const wchar_t* widecstr = widestr.c_str();

	settings->title = toVector(widecstr);
	settings->width = this->m_width;
	settings->height = this->m_height;



	bool success = DrawScatterPlotFromSettings(canvasReference, settings, errorMessage);
	if (!success) {
		throw std::runtime_error("could not draw plot");
	}
	this->largeStocksPlot = new unsigned char[m_width * m_height * 4];
	for (uint32_t x = 0; x < m_width; x++) {
		for (uint32_t y = 0; y < m_height; y++) {
			int position = (x + y * m_width) * 4;
			uint32_t x_inverse = x ;
			uint32_t y_inverse = m_height - (y + 1);
			this->largeStocksPlot[position] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->r * 255;
			this->largeStocksPlot[position + 1] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->g * 255;
			this->largeStocksPlot[position + 2] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->b * 255;
			this->largeStocksPlot[position + 3] = canvasReference->image->x->at(x_inverse)->y->at(y_inverse)->a * 255;
		}
	}

}

void Screen::DrawStockGraph() {
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, this->largeStocksPlot);
}

void Screen::DrawBackgrounImage() {
	if (this->intelBackgroundImageBuffer == nullptr) {
		std::cout << "nullptr image" << std::endl;
		return;
	}
	glDrawPixels(m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, this->intelBackgroundImageBuffer);
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
		case ScreenState::Fourth:
			this->DrawStockGraph();
			return this->FourthScreenRender();
		case ScreenState::Fifth:
			this->DrawStockGraph();
			return this->FifthScreenRender();
		case ScreenState::Sixth:
			this->DrawStockGraph();
			return this->SixthScreenRender();
		default:
			std::cout << "Default hit in screen render function. Terminating" << std::endl;
			throw std::runtime_error("Default hit in screen render function. Terminating");
	}
}
