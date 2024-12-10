#include <openmv.h>

bool OpenMV::checkUART(AprilTagDatum& tag)  
{
  bool retVal = false;
  while(port.available())
  {
    uint8_t b = port.read();
    if(handleUART(b))
    {
      memcpy(&tag, &mvArray, sizeof(AprilTagDatum));
      retVal = true;
    }
  }

  return retVal;
}

bool OpenMV::handleUART(uint8_t b)
{
  bool retVal = false;
  switch(mvIndex)
  {
    case 0:
      if(b == 0xff) mvIndex++; //first byte must be 0xff
      break;
    case 1:
      if(b == 0x55) mvIndex++;
      else mvIndex = 0; //didn't get the 0x55 byte, so restart
      break;
    case sizeof(AprilTagDatum):
      if(b == 0xaa) //correct end byte, so process
      {
        retVal = true;
        mvIndex = 0;
      } 
      else mvIndex = 0; //didn't get the aa byte, so restart
      break;
    case sizeof(AprilTagDatum) + 1:
      Serial.println("Something is very wrong!"); //PANIC
      break;
    default:
      mvArray[mvIndex++] = b;
  }

  //TODO: add checksum verification

  return retVal;
}

