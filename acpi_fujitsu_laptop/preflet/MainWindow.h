#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <Box.h>
#include <StringView.h>
#include <Slider.h>
#include <Window.h>

typedef struct {
	unsigned int backlight_limit;
	int backlight_level;
} acpi_fujitsu_laptop;

class MainWindow : public BWindow
{
public:
								MainWindow();
	virtual						~MainWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			void				_AdjustInterface();
	static	void				_DeviceStatusPrettyName(char *, unsigned int);

			status_t			_OpenDevice();
			status_t			_CloseDevice();
			status_t			_GetStatus(bool);
			void				_SaveSettings();

			acpi_fujitsu_laptop fStatus;

			BBox*				fBacklightBox;
			BSlider*			fBacklightSlider;

			int					fDevice;
};

#endif	/* MAINWINDOW_H */
