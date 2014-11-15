#include "FujitsuLaptop.h"
#include "MainWindow.h"

const char* kSignature = "application/x-vnd.Haiku-FujitsuLaptop";

FujitsuLaptop::FujitsuLaptop()
	: BApplication(kSignature)
	, fMainWindow(NULL)
{
}

FujitsuLaptop::~FujitsuLaptop()
{
}

void
FujitsuLaptop::ReadyToRun()
{
	fMainWindow = new MainWindow();
}

void
FujitsuLaptop::AboutRequested()
{
	// TODO
}

int
main(int, char**)
{
	FujitsuLaptop app;
	app.Run();

	return 0;
}