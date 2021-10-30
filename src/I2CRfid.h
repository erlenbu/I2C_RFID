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
    RFIDSlaveObject(uint8_t slave_no) : m_SlaveNumber(slave_no) { }
    ~RFIDSlaveObject() {}
    inline int getRFIDStatus()   { return makeRequest(I2C_Request::STATUS); }
    inline int getReaderAmount() { return makeRequest(I2C_Request::SENSOR_AMOUNT);   }
    inline int clearUidCache() { return makeRequest(I2C_Request::CLEAR_UID_CACHE);   }


  private:
    uint8_t m_SlaveNumber;

    int makeRequest(I2C_Request request);
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

    void addRfidReader(uint8_t ss_pin, uint8_t rst_pin, void(*callback)(int,bool), UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}});

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

      static void I2CWrite(uint8_t data);

      static void receiveEvent(int num_bytes);

      static void requestEvent();

      static  void tagChangeEvent(int id, bool state);
  };
}
#endif