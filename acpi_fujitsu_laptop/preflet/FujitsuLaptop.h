#ifndef FUJITSU_H
#define FUJITSU_H

#include <Application.h>
#include "MainWindow.h"


class FujitsuLaptop : public BApplication
{
public:
								FujitsuLaptop();
	virtual						~FujitsuLaptop();

	virtual	void				ReadyToRun();
	virtual void				AboutRequested();

private:
			MainWindow*			fMainWindow;
};

#endif
