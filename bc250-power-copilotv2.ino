#include "PowerController.h"

PowerController controller;

void setup() {
  controller.begin();
}

void loop() {
  controller.update();
}