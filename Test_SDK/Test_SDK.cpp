// Test_SDK.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <stdio.h>
#include <string>
#include <windows.h>

//myo
#include <array>
#include <sstream>
#include <stdexcept>
#include <myo/myo.hpp>

//print to text
#include <iostream>
#include <iomanip>
#include <fstream>
#include <time.h>
#include <ctime>
#include <stack>
#include <math.h>

//include dataglove class
#include "VHand30.h"

using namespace std;

std::stack<clock_t> tictoc_stack;   // adding clock_t to the stack
ofstream myfile;
ofstream myfile2;

class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		: emgSamples()
	{
		myfile.open("MyoData.txt");//, ios::out | ios::app | ios::ate);
	}

	~DataCollector() {
		myfile.close();
	}

	void tic() {
		tictoc_stack.push(clock());
	}

	void toc() {
		std::cout << "Time elapsed: "
			<< ((double)(clock() - tictoc_stack.top())) / CLOCKS_PER_SEC
			<< std::endl;
		tictoc_stack.pop();
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		emgSamples.fill(0);
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{
		for (int i = 0; i < 8; i++) {
			emgSamples[i] = emg[i];
		}
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.


	void print()
	{
		// Clear the current line
		std::cout << '\r';

		// Print out the EMG data.
		for (size_t i = 0; i < emgSamples.size(); i++) {
			std::ostringstream oss;
			oss << static_cast<int>(emgSamples[i]);
			std::string emgString = oss.str();
			std::cout << '[' << emgString << std::string(4 - emgString.size(), ' ') << ']';
			myfile << emgString << ' ';
		}
		std::cout << std::flush;
	}

	// The values of this array is set by onEmgData() above.
	std::array<int8_t, 8> emgSamples;
private:
	// We define this function to print the current values that were updated by the on...() functions above.

};
int _tmain(int argc, _TCHAR* argv[])
{
	//create a new dataglove instance
	VHand30 *dataglove = new VHand30;

	//open text file and time variables
	myfile2.open("VMGData.txt");
	double timeElasped = 0.000;

	//set dataglove connection parameter (COM port nt important, IP address got from VHand3Manager)
	//dataglove->SetConnectionParameters(52,"192.168.2.239");
	dataglove->SetConnectionParameters(1, "127.0.0.1");

	//connect with the USB connection, created comport = 1
	int ret = dataglove->Connect(CONN_USB, STREAM_FINGERS_QUATERNION);
	fprintf(stderr, "CONNECT RET: %d\n", ret);

	//wait for connection established
	long start = ::GetTickCount();
	while ((dataglove->Connected == NOT_CONNECTED))
	{
		if ((::GetTickCount() - start) > 5000) {
			//timeout, error with the connection
			fprintf(stderr, "Cannot connect to the dataglove\n");
			dataglove->Disconnect();
			return 0;
		}
	}

	//show dataglove informations
	char label[VHAND_STRLEN];
	int ID = 0;
	dataglove->GetID(label, &ID);
	fprintf(stderr, "LABEL:%s ID:%d\n", label, ID);

	//firmware version
	int fw1, fw2, fw3;
	dataglove->GetFWVersion(&fw1, &fw2, &fw3);
	fprintf(stderr, "FIRMWARE:%d.%d.%d\n", fw1, fw2, fw3);

	//access point configuration
	char ssid[VHAND_STRLEN], pwd[VHAND_STRLEN];
	dataglove->GetAPNSettings(ssid, pwd);
	fprintf(stderr, "SSID: %s PWD:%s\n", ssid, pwd);

	//show network information
	char ip[VHAND_STRLEN], gw[VHAND_STRLEN], nm[VHAND_STRLEN];
	int DHCP;
	dataglove->GetWiFiSettings(ip, nm, gw, &DHCP);
	fprintf(stderr, "IP: %s NM:%s GW:%s DHCP:%d\n", ip, nm, gw, DHCP);

	// We catch any exceptions that might occur below -- see the catch statement for more details.

	// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
	// publishing your application. The Hub provides access to one or more Myos.
	myo::Hub hub("om.example.emg-data-sample");
	std::cout << "Attempting to find a Myo..." << std::endl;

	// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
	// immediately.
	// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
	// if that fails, the function will return a null pointer.
	myo::Myo* myo = hub.waitForMyo(10000);

	// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
	if (!myo) {
		throw std::runtime_error("Unable to find a Myo!");
	}

	// We've found a Myo.
	std::cout << "Connected to a Myo armband!" << std::endl << std::endl;

	// Next we enable EMG streaming on the found Myo.
	myo->setStreamEmg(myo::Myo::streamEmgEnabled);

	// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
	DataCollector collector;
	//const double minMax[10] = { 32, 88, 36, 104, 37, 111, 36, 105, 36, 96 }; //T.I.M.R.P
																		 // Hub::run() to send events to all registered device listeners.
	hub.addListener(&collector);

	myfile2 << std::fixed;
	myfile2 << std::setprecision(3);
	myfile << std::fixed;
	myfile << std::setprecision(3);

	//while ((::GetTickCount()-start)<10000){
	while (1) {
		//Sleep(100);
		collector.tic();
		hub.run(1000 / 100);
		int connstatus = dataglove->GetConnectionStatus();
		//is the dataglove conected with the Wifi interface?
		if (connstatus == USB_CONNECTED) {
			double fing[5];
			double roll, pitch, yaw;
			dataglove->GetFingers(fing);
			dataglove->GetAttitude(&roll, &pitch, &yaw);
			unsigned int time = dataglove->GetLastPackageTime();
			//fprintf(stderr,"TIME: %04d\n",time);
			//fprintf(stderr, "F1:%.1f F2:%1.f F3:%.1f F4:%.1f F5:%.1f\n", fing[0], fing[1], fing[2], fing[3], fing[4]);
			fprintf(stderr, "F1:%.1f F3:%.1f\n", fing[1], fing[3]);
			myfile2 << fing[1] << " " << fing[3] << " ";
			
			//fprintf(stderr,"ROLL:%.1f PITCH:%.1f YAW:%.1f\n",roll,pitch,yaw);
		}
		collector.print();
		timeElasped = timeElasped + ((double)(clock() - tictoc_stack.top())) / CLOCKS_PER_SEC;
		tictoc_stack.pop();
		myfile << timeElasped << endl;
		myfile2 << timeElasped << endl;
	}
	myfile.close();
	//disconnect socket
	dataglove->Disconnect();
	Sleep(1000);
	//turn off module
	dataglove->TurnOFF(CONN_USB);
	Sleep(1000);
	//exit
	return 0;
	//}
	/*catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}*/
}
