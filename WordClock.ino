#include <Sprite/Sprite.h>
#include "Symbols.h"
#include "Matrix.h"
#include "Button.h"
#include "PushButton.h"

#define MATRIX_WIDTH 14
#define MATRIX_HEIGHT 14

Matrix display = Matrix(MATRIX_WIDTH, MATRIX_HEIGHT);

PushButton btnUp = PushButton(4);
PushButton btnMain = PushButton(5);
PushButton btnDown = PushButton(6);

void setup()
{
	btnMain.onHold(1000, 250, mainButtonHold);
}

void loop()
{

	// Update the display; handle multiplexing etc.
	display.loop();
}

void mainButtonHold(Button*, uint16_t duration){
	
}