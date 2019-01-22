// Visual Micro is in vMicro>General>Tutorial Mode
// 
/*
	Name:       Load v0.36.ino
	Created:	20.01.2019 1:41:30
	Author:     DESKTOP-V3S93CM\Ilya
*/


#include <NokiaLCD.h>
#include <GyverEncoder.h>

//пины для энкодера
#define CLK 2
#define DT 9
#define SW 8
Encoder enc1(CLK, DT, SW);

//пины для экрана
NokiaLCD NokiaLCD(3, 4, 5, 6, 7); // (SCK, MOSI, DC, RST, CS)
#define LCD_ROWS 6 				  // количество строк на экране
#define LCD_MAX_LENGHT 15		  // максимальная длина строки в меню

#define MAX_AMPS 150.0
#define DEFAULT_VOLTS 150

//настройки
unsigned short tKrit = 60, maxTime = 180, iConst_Time = 10;
float iConst_I = 0.0, iConst_V = 50.0;

char char_iConst_I[12] = { "I   =  " }, char_iConst_V[12] = { "Vmin=     " }, char_iConst_Time[12] = { "Time=     " };

//прототипы функций
void infoMenuFunc();
void iConstSetVFunc();

class Menu
{
public:
	char *Name; 									// название меню
	Menu *Parent = NULL; 							// указатель на родителя
	unsigned short childNumb = 0; 					// количество потомков
	Menu *childs[5]; 								// указатели на потомков
	unsigned short pointerPosition = 1;				// позиция указателя
	void(*endAction)() = NULL;						// указатель на действия для конечных пунктов
	Menu* action()									// функция действия
	{

		if (this->pointerPosition == 0)
		{
			if (this->Parent != NULL)
			{
				Serial.println("ACTION: Try to Show Parent Menu");
				this->Parent->showMenu();
				return this->Parent;
			}
			else
			{
				return this;
			}
		}
		else
		{
			//Serial.println("Pointer pos isnt NULL");
			if (!(this->childs[(this->pointerPosition - 1)]->childNumb))
			{
				Serial.println("ACTION: There is no childs. Try to make end action");
				if (this->childs[this->pointerPosition - 1]->endAction != NULL)
				{
					this->childs[this->pointerPosition - 1]->endAction();
					this->showMenu();
				}
				else
				{
					Serial.println("WARNING: This is end point of menu, but there is no end function!");
				}
				return this;
			}
			else
			{
				Serial.println("ACTION: Try to Show child Menu");
				this->childs[this->pointerPosition - 1]->showMenu();
				return this->childs[this->pointerPosition - 1];
			}
		}
	}
	void addChild(Menu *child)
	{
		Serial.print("Adding new child for ");
		Serial.println(this->Name);
		this->childNumb++;
		this->childs[childNumb - 1] = child;
	}
	void addParent(Menu *newParent)
	{
		Serial.print("Adding new parent for ");
		Serial.println(this->Name);
		this->Parent = newParent;
		this->Parent->addChild(this);
		Serial.print("Parent added successfull\n\n");
	}
	void showMenu()
	{
		NokiaLCD.clear();
		if (this->Parent == NULL)
		{
			NokiaLCD.setCursor(8, 0);
			NokiaLCD.print(this->Name);
		}
		else
		{
			NokiaLCD.setCursor(8, 0);
			NokiaLCD.print("<< BACK");
		}
		for (int i = 0; i < childNumb; i++)
		{
			NokiaLCD.setCursor(8, i + 1);
			NokiaLCD.print(this->childs[i]->Name);
		}
		showPointer();
	}
	void showPointer()
	{
		for (int i = 0; i < LCD_ROWS; i++)
		{
			NokiaLCD.setCursor(0, i);
			NokiaLCD.print(" ");
		}

		NokiaLCD.setCursor(0, this->pointerPosition);
		NokiaLCD.print(">");
	}
	int movePointer(int movement)
	{
		if (movement > 0)
		{
			this->pointerPosition = (this->pointerPosition + movement) % (this->childNumb + 1);
		}
		else
		{
			if ((this->pointerPosition + movement < 0) || (this->pointerPosition + movement > this->childNumb))
				this->pointerPosition = childNumb;
			else
				this->pointerPosition = this->pointerPosition + movement;
		}
		showPointer();
	}
};

Menu mainMenu, infoMenu, loadMenu, settingsMenu, iConst, pConst, iConstSetI, iConstSetV, iConstSetTime, iConstStart;
Menu *_this = &mainMenu;
void setup()
{
	Serial.begin(9600);
	//enc1.setTickMode(AUTO); //авторежим работы энкодера
	NokiaLCD.init();        // инициализация экрана
	NokiaLCD.clear();       // очистка экрана

	mainMenu.Name = "MAIN";

	loadMenu.Name = "Load";
	loadMenu.addParent(&mainMenu);

	infoMenu.Name = "Info";
	infoMenu.addParent(&mainMenu);
	infoMenu.endAction = infoMenuFunc;

	settingsMenu.Name = "Settings";
	settingsMenu.addParent(&mainMenu);

	iConst.Name = "I = const";
	iConst.addParent(&loadMenu);

	pConst.Name = "P = const";
	pConst.addParent(&loadMenu);


	//sprintf(char_iConst_V, "S0_%d", iConst_V);
	dtostrf(iConst_V, 5, 2, &char_iConst_V[6]);
	iConstSetV.Name = char_iConst_V;
	iConstSetV.addParent(&iConst);
	iConstSetV.endAction = iConstSetVFunc;
	iConstSetI.endAction = iConstSetIFunc;

	dtostrf(iConst_I, 5, 2, &char_iConst_I[6]);
	iConstSetI.Name = char_iConst_I; //установка тока для режима I = contant
	iConstSetI.addParent(&iConst);

	dtostrf(iConst_Time, 3, 0, &char_iConst_Time[6]);
	iConstSetTime.Name = char_iConst_Time; //установка времени для режима I = const (возможно стоит перенести в настройки)
	iConstSetTime.addParent(&iConst);


	mainMenu.showMenu();
}


void loop()
{
	enc1.tick();
	if (enc1.isLeft())
	{
		_this->movePointer(-1);
		Serial.print("LEFT: pointerPosition = ");
		Serial.println(_this->pointerPosition);
	}
	if (enc1.isRight())
	{
		_this->movePointer(1);
		Serial.print("RIGHT: pointerPosition = ");
		Serial.println(_this->pointerPosition);
	}
	if (enc1.isPress())
	{
		Serial.println("Press");
		_this = _this->action();
	}
	//Serial.print("SIZE = ");
	//Serial.println(sizeof(int));
}

void infoMenuFunc()
{
	Serial.println("infoMenuFunc is started");
	NokiaLCD.clear();
	NokiaLCD.setCursor(18, 0);
	NokiaLCD.print("VERSION");
	NokiaLCD.setCursor(24, 2);
	NokiaLCD.print("v0.37");
	NokiaLCD.setCursor(8, 4);
	NokiaLCD.print("21.11.2019");
	bool exitFlag = 1;
	while (exitFlag)
	{
		enc1.tick();
		if (enc1.isPress())
		{
			Serial.println("Press");
			exitFlag = 0;
		}
	}
}

void iConstSetVFunc()
{
	Serial.println("Huli ti ne rabotaesh");
	float tmp_volts = iConst_V;
	boolean exit = 1;
	boolean saveChanges = false;
	while (exit) 
	{
		enc1.tick();
		if (enc1.isLeft())
		{
			tmp_volts -= 0.1;
		}
		if (enc1.isRight())
		{
			tmp_volts += 0.1;
		}
		if (enc1.isPress())
		{
			NokiaLCD.clear();
			NokiaLCD.setCursor(25, 0);
			NokiaLCD.print("SAVE");
			NokiaLCD.setCursor(15, 1);
			NokiaLCD.print("CHANGES?");
			NokiaLCD.setCursor(5, 2);
			NokiaLCD.print((iConstSetV.Name));
			while (exit)
			{
				Serial.print("NAME  :  ");
				Serial.println(iConstSetV.Name);
				enc1.tick();
				NokiaLCD.setCursor(10, 4);
				NokiaLCD.print("YES");
				NokiaLCD.setCursor(60, 4);
				NokiaLCD.print("NO");
				if (enc1.isRight())
				{
					NokiaLCD.setCursor(2, 4);
					NokiaLCD.print(" ");
					NokiaLCD.setCursor(52, 4);
					NokiaLCD.print(">");
					saveChanges = false;
				}
				if (enc1.isLeft())
				{
					NokiaLCD.setCursor(52, 4);
					NokiaLCD.print(" ");
					NokiaLCD.setCursor(2, 4);
					NokiaLCD.print(">");
					saveChanges = true;
				}
				if (enc1.isPress())
				{
					exit = false;
				}
			}
		}
		if (saveChanges)
		{
			iConst_V = tmp_volts;
		}
		else
		{
			if (!exit)
			{
				dtostrf(iConst_V, 5, 2, &char_iConst_V[6]);
				iConstSetV.Name = char_iConst_V;
			}
			else
			{
				dtostrf(tmp_volts, 5, 2, &char_iConst_V[6]);
				iConstSetV.Name = char_iConst_V;
			}
		}
		NokiaLCD.setCursor(8, 1);
		NokiaLCD.print(iConstSetV.Name);
	}
	Serial.print("V_min = ");
	Serial.println(iConst_V);
}

void iConstSetIFunc()
{
	Serial.println("iConstSetIFunc is started");
	float tmp_Amps = iConst_I;
	boolean exit = 1;
	boolean saveChanges = false;
	while (exit)
	{
		enc1.tick();
		if (enc1.isLeft())
		{
			tmp_Amps -= 0.1;
		}
		if (enc1.isRight())
		{
			tmp_Amps += 0.1;
		}
		if (enc1.isPress())
		{
			NokiaLCD.clear();
			NokiaLCD.setCursor(25, 0);
			NokiaLCD.print("SAVE");
			NokiaLCD.setCursor(15, 1);
			NokiaLCD.print("CHANGES?");
			NokiaLCD.setCursor(5, 2);
			NokiaLCD.print((iConstSetI.Name));
			while (exit)
			{
				Serial.print("NAME  :  ");
				Serial.println(iConstSetI.Name);
				enc1.tick();
				NokiaLCD.setCursor(10, 4);
				NokiaLCD.print("YES");
				NokiaLCD.setCursor(60, 4);
				NokiaLCD.print("NO");
				if (enc1.isRight())
				{
					NokiaLCD.setCursor(2, 4);
					NokiaLCD.print(" ");
					NokiaLCD.setCursor(52, 4);
					NokiaLCD.print(">");
					saveChanges = false;
				}
				if (enc1.isLeft())
				{
					NokiaLCD.setCursor(52, 4);
					NokiaLCD.print(" ");
					NokiaLCD.setCursor(2, 4);
					NokiaLCD.print(">");
					saveChanges = true;
				}
				if (enc1.isPress())
				{
					exit = false;
				}
			}
		}
		if (saveChanges)
		{
			iConst_I = tmp_Amps;
		}
		else
		{
			if (!exit)
			{
				dtostrf(iConst_I, 5, 2, &char_iConst_I[6]);
				iConstSetV.Name = char_iConst_I;
			}
			else
			{
				dtostrf(tmp_Amps, 5, 2, &char_iConst_I[6]);
				iConstSetI.Name = char_iConst_I;
			}
		}
		NokiaLCD.setCursor(8, 2);
		NokiaLCD.print(iConstSetI.Name);
	}
	Serial.print("V = ");
	Serial.println(iConst_I);
}