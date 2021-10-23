#ifndef Communication_h
#define Communication_h
#include <Wire.h>
#include <Arduino.h>

enum I2C_Request {
  INIT = 0,
  STATUS = 1
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
    inline int getReaderAmount() { return makeRequest(I2C_Request::INIT);   }

  private:
    uint8_t m_SlaveNumber;

    int makeRequest(I2C_Request request) {
      int data;

      Wire.beginTransmission(m_SlaveNumber);

      size_t byte_size = Wire.write(request);
      if(1 != byte_size) {
        Serial.print("Wrote wrong byte amount to slave?"); Serial.println(byte_size);
        return -1;
      }
      uint8_t status = Wire.endTransmission();
      if(0 != status) {
        Serial.print("ERROR: Wire::endTransmission() returned status "); Serial.println(status);
        return -1;
      }

      int bytes = Wire.requestFrom((int)m_SlaveNumber, 1);
      //RFID status is 1 byte long
      if(1 != bytes) {
        Serial.print("RFID status wrong length: "); Serial.println(bytes);
        return -1;
      }

      while (Wire.available()) {
        data = Wire.read();
        if(-1 == data) {
          Serial.println("ERROR: Wire::read() returned -1");
          return -1;
        }
        // Serial.println(data, BIN);     
      }

      return data;
    }
  };

  class I2CRfidMaster
  {
  public:
      I2CRfidMaster()
      : m_SlaveAmount(0)
      {
        Wire.begin();
        m_SlaveArray = (RFIDSlaveObject*)malloc(sizeof(RFIDSlaveObject));
        m_SlaveArrayMapping = (uint8_t*)malloc(sizeof(uint8_t));
      }

      ~I2CRfidMaster()
      {
        free(m_SlaveArray);
        free(m_SlaveArrayMapping);
        free(m_SlaveAmount);
      }

    inline uint8_t getSlaveAmount() { return m_SlaveAmount; }

    void addI2CSlave(uint8_t slave_no)
    {
      m_SlaveAmount += 1;
      m_SlaveArray = (RFIDSlaveObject*)realloc(m_SlaveArray, m_SlaveAmount * sizeof(RFIDSlaveObject));
      m_SlaveArray[m_SlaveAmount-1] = RFIDSlaveObject(slave_no);
      // m_SlaveArray[m_SlaveAmount-1].hello();
      m_SlaveArrayMapping = (uint8_t*)realloc(m_SlaveArrayMapping, m_SlaveAmount * sizeof(uint8_t));
      m_SlaveArrayMapping[m_SlaveAmount-1] = slave_no;
      Serial.print("Amount of slaves: "); Serial.println(m_SlaveAmount);
    }

    uint8_t getSlaveArrayElement(uint8_t slave_no)
    {
      int element = -1;
      for(uint8_t a = 0; a < m_SlaveAmount; a++) {
        if(m_SlaveArrayMapping[a] == slave_no)
          element = a;
      }

      if(element == -1) {
        Serial.print("Slave id "); Serial.print(slave_no); Serial.println(" is not known");
      }
      return element;
    }

    int getSlaveRFIDStatus(uint8_t slave_no)
    {
      int status = -1;
      int element = getSlaveArrayElement(slave_no);
      if(element != -1) {
        status = m_SlaveArray[element].getRFIDStatus();
      }
      else {
        Serial.println("Invalid slave number");
      }

      return status;
    }

    int getSlaveRFIDSensorAmount(uint8_t slave_no) {
      int element = getSlaveArrayElement(slave_no);
      int sensorAmount = -1;
      if(element != -1) {
        sensorAmount = m_SlaveArray[element].getReaderAmount();
      }
      else {
        Serial.println("Invalid slave number");
      }
      return sensorAmount;
    }

  private:
    RFIDSlaveObject* m_SlaveArray;
    uint8_t* m_SlaveArrayMapping;
    uint8_t m_SlaveAmount;
  };

}

/*********************  Slave side *********************/

namespace I2CSlave
{
  class I2CRfidSlave
  {

  };
}
#endif