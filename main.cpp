//#define DEBUG

#include <cstdio>
#include <iostream>
#include <wiringPi.h>
#include <unistd.h>
#include <chrono>
#include <ctime>    
#include <array>
#include <memory>
#include <vector>

#define DOUBLECLICKTIMERINTERVALMS 750

#define PIN 8
double timeoutTime = 60.0;

std::string exec(const char* cmd) {
	std::array<char, 128> buffer;
	std::string result;
	std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
	if (!pipe) {
		throw std::runtime_error("popen() failed!");
	}
	while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
		result += buffer.data();
	}
	return result;
}

std::vector<std::string> split(const std::string& str, const std::string& delim)
{
	std::vector<std::string> tokens;
	size_t prev = 0, pos = 0;
	do
	{
		pos = str.find(delim, prev);
		if (pos == std::string::npos) pos = str.length();
		std::string token = str.substr(prev, pos - prev);
		tokens.push_back(token);

		prev = pos + delim.length();
	} while (pos < str.length() && prev < str.length());
	return tokens;
}

bool IsScreenOn()
{
	std::string returnedValue = exec("vcgencmd display_power");
	std::vector<std::string> returnedValueVector = split(returnedValue, "=");

#ifdef DEBUG
	std::cout << returnedValue << std::endl;
#endif // DEBUG


	if (returnedValueVector.size() == 2 && returnedValueVector[1] == "1\n")
		return true;

	return false;
}

void ChangeBrowserTab() // Require installed xdotool
{
#ifdef DEBUG
	system("xdotool key ctrl+Tab");
	std::cout << "Switched tabs" << std::endl;
#else
	system("xdotool key ctrl+Tab >> /dev/null");
#endif // DEBUG
}

uint32_t numOfClicks = 0;
std::chrono::_V2::system_clock::time_point clicksStart;

void PinValueChanged()
{
	if (!digitalRead(PIN))
	{
#ifdef DEBUG
		std::cout << "PIN value: " << digitalRead(PIN) << std::endl;
		std::cout << "Detected" << std::endl;
#endif // DEBUG

		if (IsScreenOn()) // Button/PIR "double-click"
		{
			std::chrono::milliseconds elapsedTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - clicksStart);

#ifdef DEBUG
			std::cout << "Elapsed time: " << elapsedTime.count() << " ms" << std::endl;
#endif // DEBUG

			if (numOfClicks == 0 || elapsedTime.count() > DOUBLECLICKTIMERINTERVALMS)
			{
				clicksStart = std::chrono::system_clock::now();
				numOfClicks = 0;
			}

			++numOfClicks;

			if (numOfClicks == 2 && elapsedTime.count() <= DOUBLECLICKTIMERINTERVALMS)
			{
				ChangeBrowserTab();

				numOfClicks = 0;
			}
		}

#ifdef DEBUG
		std::cout << "Screen turned on" << std::endl;
#endif // DEBUG
	}
	else
	{
#ifdef DEBUG
		std::cout << "PIN value: " << digitalRead(PIN) << std::endl;
		std::cout << "Undetected" << std::endl;
#endif // DEBUG

	}
}

int main(int argc, char* argv[])
{
	wiringPiSetup();
	pinMode(PIN, INPUT);

	// Bind to interrupt
	wiringPiISR(PIN, INT_EDGE_BOTH, &PinValueChanged);

	while (1)
	{
		sleep(1);
	}

	return 0;
}