#include <Windows.h>
#include <iostream>
#include <string>
#include <urlmon.h>
#include <regex>
#include <fstream>
#include <thread>
#include <json.hpp>

#pragma comment(lib, "urlmon.lib")

using json = nlohmann::json; // Using directive is okay

class WeatherApp {
private:

    enum class TemperatureSystem : char { // Use char as its clearer to read and uses less data

        imperial = 'F',
        metric = 'C'

    };

    struct CityData {

        TemperatureSystem currentTempType;
        std::string cityName;
        int temperature;

    }; // Probably better to make cityData & temp into a seperate class and just create an instance here

    CityData cityData;

public:

    std::string createURL(const std::string& cityName, const std::string& tempSystem) const { // For use with URLDownloadToFile

        std::string URL{ "https://api.openweathermap.org/data/2.5/find?q=" + std::regex_replace(cityName, std::regex(" "), "+") + "&units=" + tempSystem + "&type=accurate&mode=JSON&APPID=a59eb03f93dd3e6d623b3622799eac70" };
        return URL;

    }

    bool downloadJSON(const std::string& URL, std::string fileName) const {

        std::wstring urlTMP(URL.begin(), URL.end());
        LPCWSTR wURL = urlTMP.c_str();
        fileName += ".JSON";
        std::wstring fileNameTMP(fileName.begin(), fileName.end());
        LPCWSTR wFileName = fileNameTMP.c_str();

        HRESULT result = URLDownloadToFile(NULL, wURL, wFileName, 0, NULL);

        switch (result) {

            case S_OK: {           
                return true;              
            }
            default: {
                printf("\nError connecting to the API! %d\nLast error: %d\n", result, GetLastError());
                return false;
            }
        }
    }

    int findTemp() {

        std::ifstream file{ "City.JSON" };
        json city { json::parse(file) };
        int temp { city["list"][0]["main"]["temp"] };
        file.close();
        return temp;

    }

    bool checkCityName() const { // Check if city exists

        std::ifstream file{ "City.JSON" };
        json city{ json::parse(file) };
        int count{ city["count"] };
        file.close();
        return count > 0;

    }

    void populateCityInfo() {

        std::string cityName{ getInputCityName("\nEnter city name : ") }; // We must store it to find out whether or not user input is the same as the cityName data member already stored, else we would be able to call setCityName with GetInput
        TemperatureSystem tempType{ getInputTempSystem("\nEnter Temperature System : F for Fahrenheit, C for Celcius : ") };

        if (cityName == getCityName() && tempType == getTempSystem()) {
        
            return;

        }

        setCityName(cityName);
        setTempType(tempType);
        std::string URL = createURL(getCityName(), getTempSystemAsString());

        if (downloadJSON(URL, "City") ) { // If API works, even if the cityname doesnt
        
            while (!checkCityName()) { // No need to check if downloadJSON worked properly, as if it worked the first time, itll work now
            
                setCityName(getInputCityName("\nError! No city found! Please enter a valid city name : "));
                std::string newURL = createURL(getCityName(), getTempSystemAsString());
                downloadJSON(newURL, "City");

            }

            setCityTemp(findTemp());
            std::thread t1(&WeatherApp::keepTempUpdated, this);
            t1.detach();

        }

    }

    void keepTempUpdated() { // will always be ran on a seperate thread

        std::string cityToKeepUpdated{ getCityName() };
        TemperatureSystem temperatureSystemToKeepUpdated{ getTempSystem() };
        int seconds{ 0 };

        while (cityToKeepUpdated == getCityName() && temperatureSystemToKeepUpdated == getTempSystem() ) {

            Sleep(1000);
            ++seconds;

            if (seconds == 120) {
                seconds = 0;
                setCityTemp(findTemp());
            }
        }
        
    }

    std::string geolocateCity() const {

        std::string cityName{ "" };
        downloadJSON("http://ipwho.is/", "CurrentCity");
        std::ifstream ip{ "CurrentCity.JSON" };
        json currentCity{ json::parse(ip) };
        cityName = currentCity["city"];
        ip.close();
        return cityName;

    }

    void printCityData() const {
    
        std::cout << "\n\nCITY NAME : " << getCityName() << "\nCITY TEMP : " << getCityTemp();

    }

    void setCityName(const std::string& cityName) {

        cityData.cityName = cityName;

    }

    const std::string& getCityName() const {

        return cityData.cityName;

    }

    void setTempType(TemperatureSystem tempSystem) {
    
        cityData.currentTempType = tempSystem;

    }

    std::string getTempSystemAsString() const {

        switch (cityData.currentTempType) {
        
            case TemperatureSystem::imperial: 
                return "imperial";          
            case TemperatureSystem::metric: 
                return "metric";
            default: 
                return "NULL";

        }

    }

    TemperatureSystem getTempSystem() {
    
        return cityData.currentTempType;

    }

    int getCityTemp() const {
    
        return cityData.temperature;

    }

    void setCityTemp(int temp) {
    
        cityData.temperature = temp;

    }

    int menu() const {

        int choice;
        std::cout << "\n\n1. Get Temperature\n2. Exit\n\nChoice : ";
        std::cin >> choice;
        return choice;

    }

    std::string getInputCityName(const std::string&& message) const {

        std::string input{""};
        std::cout << message;
        std::cin.ignore(); // Clear buffer, we must do this as we got cin >> var input before, and we cant use std::cin >> std::ws to discard white spaces as then we wont be able to enter an empty space
        std::getline(std::cin, input);
        if (input.size() == 0) {
            input = geolocateCity(); // If user does not enter a city, we will attempt to geolocate them
        }
        return input;

    }

    TemperatureSystem getInputTempSystem(const std::string&& message) const {

        char input;
        std::cout << message;
        std::cin >> input;
        return static_cast<TemperatureSystem>(toupper(input));

    }

};


int main() {

    WeatherApp w;
    
    bool repeat{ true };

    while (repeat) {
    
        switch (w.menu()) {
        
        case 1: 

            w.populateCityInfo();
            w.printCityData();
            break;

        case 2: 

            repeat = false;
    
        }
    
    }

    


    
    

}
