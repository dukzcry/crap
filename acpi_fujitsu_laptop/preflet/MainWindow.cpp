/* Written by Artem Falcon <lomka@gero.in> */

#include <LayoutBuilder.h>
#include "MainWindow.h"
//#include <Dragger.h>

#include "acpi_fujitsu_common.h"

#include <Path.h>
#include <File.h>
#include <Directory.h>
#include <FindDirectory.h>

const uint32 kMsgChangeBacklightLevel = 'cblv';

MainWindow::MainWindow()
	: BWindow(BRect(0, 0, 210, 110), "FujitsuLaptop",
		B_TITLED_WINDOW, B_NOT_RESIZABLE | B_NOT_ZOOMABLE
			| B_AUTO_UPDATE_SIZE_LIMITS | B_QUIT_ON_WINDOW_CLOSE)
	, fBacklightBox(NULL)
	, fBacklightSlider(NULL)
	, fDevice(0)
{
	fBacklightBox = new BBox("fBacklightBox");
	fBacklightBox->SetLabel("Backlight");

	fBacklightSlider = new BSlider("Level", "Level",
		new BMessage(kMsgChangeBacklightLevel), 0, 7, B_HORIZONTAL);
	fBacklightSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fBacklightSlider->SetHashMarkCount(7);
	fBacklightSlider->SetLimitLabels("0", "7");
	fBacklightSlider->SetEnabled(false);

	/* TO-DO: create BView descendant and move this to it */
	/*BRect frame;
	frame.OffsetTo(B_ORIGIN);
	frame.top = frame.bottom - 7.0f;
	frame.left = frame.right - 7.0f;
	// handle follows origin size of slider, for sure
	fBacklightSlider->AddChild(new BDragger(frame, fBacklightSlider,
		B_FOLLOW_RIGHT*/ /* handle overlaps right label, of course */ //));

	BView* view;
	view = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, 0)
			.AddGroup(B_HORIZONTAL, 0)
				.AddGroup(B_VERTICAL, 0)
					.Add(fBacklightSlider)
				.End()
				/*.AddGroup(B_VERTICAL, 0)
					//
				.End()*/
			.End()
			.SetInsets(5, 5, 5, 5)
		.End()
		.View();
	fBacklightBox->AddChild(view);

	BLayoutBuilder::Group<>(this)
		.AddGrid(1, 1)
			.Add(fBacklightBox, 0, 0)
			.SetInsets(5, 5, 5, 5)
			.End()
		.End();


	if (_OpenDevice() == B_OK) {
		//memset(fStatus, 0, sizeof(acpi_fujitsu_laptop));
		if (_GetStatus(1) == B_OK)
			_AdjustInterface();
	}

	CenterOnScreen();
	Show();
}


MainWindow::~MainWindow()
{
	_CloseDevice();
	_SaveSettings();
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kMsgChangeBacklightLevel:
		{
			int level = int(fBacklightSlider->Position() * (this->fStatus.backlight_limit-1));
			ioctl(fDevice, SET_BACKLIGHT_LEVEL, &level, NULL);
			//_GetStatus(0);
			break;
		}

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
MainWindow::_AdjustInterface()
{
	char str[5];
	sprintf(str, "%d", this->fStatus.backlight_limit-1);

	fBacklightSlider->SetLimits(0, this->fStatus.backlight_limit-1);
	fBacklightSlider->SetHashMarkCount(this->fStatus.backlight_limit);
	fBacklightSlider->SetLimitLabels("0", str);
	fBacklightSlider->SetEnabled(true);

	if (this->fStatus.backlight_limit-1)
		fBacklightSlider->SetPosition(this->fStatus.backlight_level * 1.0
			/ (this->fStatus.backlight_limit-1));
}

void
MainWindow::_DeviceStatusPrettyName(char *str, unsigned int deviceStatus)
{
	switch (deviceStatus) {
		case -1:
			strcpy(str, "<error>");
		break;
		default:
			sprintf(str, "%d", deviceStatus);
	}
}


status_t
MainWindow::_OpenDevice()
{
	fDevice = open("/dev/power/acpi_fujitsu_laptop/0", O_RDONLY);
	return fDevice > 0 ? B_OK : B_ERROR;
}


status_t
MainWindow::_CloseDevice()
{
	if (fDevice > 0)
		close(fDevice);
	return B_OK;
}

void
MainWindow::_SaveSettings()
{
	BPath path;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &path, true) < B_OK)
		return;
	path.Append("kernel/drivers");
	if (create_directory(path.Path(), 0755) < B_OK)
		return;
	path.Append("acpi_fujitsu_laptop");
	BFile file;
	if (file.SetTo(path.Path(), B_CREATE_FILE | B_WRITE_ONLY | B_ERASE_FILE) < B_OK)
		return;

	char buffer[22];
	snprintf(buffer, sizeof(buffer), "backlight_level %d\n",
		int(fBacklightSlider->Position() * (this->fStatus.backlight_limit-1)));
	file.Write(buffer, strlen(buffer));
}

status_t
MainWindow::_GetStatus(bool first)
{
	status_t status;
	char str[7];

	if (first) {
		status = ioctl(fDevice, GET_BACKLIGHT_LIMIT, &this->
			fStatus.backlight_limit, NULL);
		if (status != B_OK) {
			printf("backlight lim status: %s\n", strerror(status));
			return B_ERROR;
		}

		_DeviceStatusPrettyName(str, this->fStatus.backlight_limit);
		printf("backlight lim: %s\n", str);
	}

	status = ioctl(fDevice, GET_BACKLIGHT_LEVEL, &this->
		fStatus.backlight_level, NULL);
	if (status != B_OK) {
		printf("backlight cur status: %s\n", strerror(status));
		return B_ERROR;
	}

	_DeviceStatusPrettyName(str, this->fStatus.backlight_level);
	printf("backlight cur: %s\n", str);

	return B_OK;
}