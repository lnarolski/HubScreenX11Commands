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
#include <functional>
#include <thread>

#define DOUBLECLICKTIMERINTERVALMS 750

#define PIN 8

class later
{
public:
	bool working = false;
	template <class callable, class... arguments>
	later(int after, bool async, callable&& f, arguments&&... args)
	{
		working = true;
		std::function<typename std::result_of<callable(arguments...)>::type()> task(std::bind(std::forward<callable>(f), std::forward<arguments>(args)...));

		if (async)
		{
			std::thread([after, task]() {
				std::this_thread::sleep_for(std::chrono::milliseconds(after));
				task();
				}).detach();
		}
		else
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(after));
			task();
		}
	}

};

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

void ChangeActiveApplication() // Require installed xdotool
{
#ifdef DEBUG
	system("xdotool key alt+Tab");
	std::cout << "Changed active application" << std::endl;
#else
	system("xdotool key alt+Tab >> /dev/null");
#endif // DEBUG
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
later* timer = NULL;

void CheckNumOfClicks()
{
#ifdef DEBUG
	std::cout << "numOfClicks: " << numOfClicks << std::endl;
#endif // DEBUG

	switch (numOfClicks)
	{
	case 1:
		ChangeActiveApplication();
		break;
	case 2:
		ChangeBrowserTab();
		break;
	default:
		break;
	}

	if (timer != NULL)
		timer->working = false;
}

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
			if (timer != NULL)
			{
				if (!timer->working)
				{
					delete timer;
					timer = new later(DOUBLECLICKTIMERINTERVALMS, true, CheckNumOfClicks);
					numOfClicks = 1;
				}
				else
				{
					++numOfClicks;
				}
			}
			else
			{
				timer = new later(DOUBLECLICKTIMERINTERVALMS, true, CheckNumOfClicks);
				numOfClicks = 1;
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