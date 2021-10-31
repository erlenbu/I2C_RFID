#ifndef I2C_RFID_H
#define I2C_RFID_H

#include <WSWire.h>
#include <Arduino.h>
#include "RfidReader.h"

enum I2C_Request {
  SENSOR_AMOUNT = 0,
  STATUS = 1,
  CLEAR_UID_CACHE = 2
};




/*********************  Master side *********************/

namespace I2CMaster
{

  class RFIDSlaveObject
  {
  public:
    RFIDSlaveObject(uint8_t slave_no) : m_SlaveNumber(slave_no), m_Readers(0) {}
    ~RFIDSlaveObject() {}
    inline int clearUidCache() { return makeRequest(I2C_Request::CLEAR_UID_CACHE);   }

    int getRFIDStatus();
    int getReaderAmount();

  private:
    uint8_t m_SlaveNumber;
    uint8_t m_Readers; 

    int makeRequest(I2C_Request request, int len = 1);
  };

  class I2CRfidMaster
  {
  public:
    I2CRfidMaster();
    ~I2CRfidMaster();


    void addI2CSlave(uint8_t slave_no);

    int getSlaveRFIDStatus(uint8_t slave_no);

    int getSlaveRFIDSensorAmount(uint8_t slave_no);

    void clearSlaveCache(uint8_t slave_no);

    void addRfidReader(uint8_t ss_pin, uint8_t rst_pin, void(*callback)(int,TAG_STATUS), UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

    void clearCache();

    void read();

  private:
    RFIDSlaveObject* m_SlaveArray;
    uint8_t* m_SlaveArrayMapping;
    uint8_t m_SlaveAmount;
    RfidHandler m_RfidHandler;

    uint8_t getSlaveArrayElement(uint8_t slave_no);
  };

}

/*********************  Slave side *********************/

namespace I2CSlave
{
  class I2CRfidSlave
  {
    public:
      I2CRfidSlave(int slave_no);

      ~I2CRfidSlave();

      void addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

      static void clearCache();

      void read();

    private:
      static uint8_t m_Request;
      static uint8_t m_RfidState;
      static RfidHandler m_RfidHandler;
      static byte* m_States;

      static void I2CWrite(byte* data, uint8_t length = 1);

      static void receiveEvent(int num_bytes);

      static void requestEvent();

      static  void tagChangeEvent(int id, TAG_STATUS state);
  };
}
#endif