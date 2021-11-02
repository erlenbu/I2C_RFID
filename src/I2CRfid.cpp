#include "I2CRfid.h"

/*********************  Master side *********************/

namespace I2CMaster
{

  /******** RFIDSlaveObject ********/

  void RFIDSlaveObject::InitSlave() {
    bool error = true;
    uint8_t readerAmount;
    while(error) {

      if(-1 != readSlaveReaderAmount(readerAmount)) {
        m_ReaderAmount = readerAmount;
        error = false;
        Serial.print("Slave "); Serial.print(m_SlaveNumber); Serial.print(" reports "); Serial.print(m_ReaderAmount); Serial.println(" readers");
      }
      else {
        Serial.print("ERROR: communication with slave "); Serial.println(m_SlaveNumber);
      }

    delay(1000);
    }
  }

  int RFIDSlaveObject::makeRequest(I2C_Request request, byte* data_received, int len) {

    if(len <= 0) {
      Serial.print("ERROR makeRequest() invalid len value: "); Serial.println(len);
      return -1;
    }

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

    int bytes = Wire.requestFrom((int)m_SlaveNumber, len);
    //RFID status is 1 byte long
    if(len != bytes) {
      Serial.print("RFID status wrong length: "); Serial.println(bytes);
      return -1;
    }

    int iter = 0;
    if(data_received != nullptr)
    {
      while (Wire.available()) {
        data_received[iter] = Wire.read();
        iter++;
        if(-1 == data_received[iter]) {
          Serial.println("ERROR: Wire::read() returned -1");
          return -1;
        }
      }
    }

    return 0;
  }

  /******** I2CRfidMaster ********/

  I2CRfidMaster::I2CRfidMaster()
  : m_SlaveAmount(0), m_RfidHandler(RfidHandler())
  {
    Wire.begin();
    m_SlaveArray = (RFIDSlaveObject*)malloc(sizeof(RFIDSlaveObject));
    m_SlaveArrayMapping = (uint8_t*)malloc(sizeof(uint8_t));
    m_GlobalStatus = TAGS_MISSING;

  }

  I2CRfidMaster::~I2CRfidMaster()
  {
    free(m_SlaveArray);
    free(m_SlaveArrayMapping);
    free(m_SlaveAmount);
  }

  void I2CRfidMaster::addI2CSlave(uint8_t slave_no)
  {
    m_SlaveAmount += 1;
    m_SlaveArray = (RFIDSlaveObject*)realloc(m_SlaveArray, m_SlaveAmount * sizeof(RFIDSlaveObject));
    m_SlaveArray[m_SlaveAmount-1] = RFIDSlaveObject(slave_no);
    m_SlaveArrayMapping = (uint8_t*)realloc(m_SlaveArrayMapping, m_SlaveAmount * sizeof(uint8_t));
    m_SlaveArrayMapping[m_SlaveAmount-1] = slave_no;
    Serial.print("Amount of slaves: "); Serial.println(m_SlaveAmount);
  }

  void I2CRfidMaster::InitMaster() 
  {
    for(int slave = 0; slave < m_SlaveAmount; slave++)
    {
      m_SlaveArray[slave].InitSlave();
    }
  }

  int I2CRfidMaster::getGlobalReaderAmount()
  {
    I2C_Tags_Status tag_status[m_SlaveAmount+1]; //Slave + local status
    uint8_t num_global_readers = 0;
    num_global_readers += m_RfidHandler.getReaderAmount();
    for(uint8_t slave = 0; slave < m_SlaveAmount; slave++)
    {
      num_global_readers += m_SlaveArray[slave].getReaderAmount();
    }

    return num_global_readers;
  }

  int I2CRfidMaster::createGlobalReaderArray(byte* tag_status)
  {
    uint8_t placement = 0;
    byte rfid_state[10];
    uint8_t num_readers = m_RfidHandler.getReaderAmount();

    int status = m_RfidHandler.readRfidState(rfid_state);
    if(-1 != status)
    {
      for(int i = 0; i < num_readers; i++)
      {
        tag_status[placement+i] = rfid_state[i];
      }
      placement += num_readers;
    }

    for(int slave = 0; slave < m_SlaveAmount; slave++)
    {
      num_readers = m_SlaveArray[slave].getReaderAmount();
      int status = m_SlaveArray[slave].readRfidState(rfid_state);
      if(-1 != status)
      {
        for(int i = 0; i < num_readers; i++)
        {
          tag_status[placement+i] = rfid_state[i];
        }
        placement += num_readers;
      }
    }
  }

  int I2CRfidMaster::checkGlobalTagStatus()
  {
    int error_status = 0;
    // I2C_Tags_Status tag_status[m_SlaveAmount+1]; //Slave + local status
    uint8_t reader_number = getGlobalReaderAmount();
    byte global_reader_status[reader_number];
     for(int i = 0; i < reader_number; i++) { global_reader_status[i] = 0; }
    createGlobalReaderArray(global_reader_status);
    
    Serial.println("Global reader state");
    for(int i = 0; i < reader_number; i++)
    {
      Serial.print(global_reader_status[i]);
    }
    Serial.println();

    I2C_Tags_Status global_status = checkTagStatus(reader_number, global_reader_status);
    Serial.print("Global status: "); Serial.println(global_status);

    if(m_GlobalStatus != global_status)
    {
      m_GlobalStatus = global_status;
      Serial.print("Changed global status to: "); Serial.println(m_GlobalStatus);
    }

    return error_status;
  }

  I2C_Tags_Status I2CRfidMaster::checkTagStatus(uint8_t reader_amount, byte* data)
  {
    uint8_t reader;
    I2C_Tags_Status tags_status;

    for(reader = 0; reader < reader_amount; reader++)
    {
      if(data[reader] == TAG_STATUS::NOT_PRESENT) 
      {
        tags_status = I2C_Tags_Status::TAGS_MISSING;
        break;
      }
      else if(data[reader] == TAG_STATUS::PRESENT_UNKNOWN_TAG)
      {
        tags_status = I2C_Tags_Status::TAGS_INCORRECT;
      }
      else
      {
        if(I2C_Tags_Status::TAGS_INCORRECT != tags_status)
        {
          tags_status = I2C_Tags_Status::TAGS_CORRECT;
        }
      }
    }

    return tags_status;
  }

  void I2CRfidMaster::addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) {
    m_RfidHandler.addRfidReader(ss_pin, rst_pin, companion_tag);

  }

  void I2CRfidMaster::clearCache() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.clearCache();
    }
  }

  void I2CRfidMaster::read() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.read();
    }
  }
}

/*********************  Slave side *********************/

namespace I2CSlave
{
  uint8_t I2CRfidSlave::m_Request = 0;
  uint8_t I2CRfidSlave::m_RfidState = 0;
  RfidHandler I2CRfidSlave::m_RfidHandler = RfidHandler();
  // byte* I2CRfidSlave::m_States = nullptr;

  I2CRfidSlave::I2CRfidSlave(int slave_no) {
    Wire.begin(slave_no);
    Wire.onRequest(requestEvent);
    Wire.onReceive(receiveEvent);
    m_Request = I2C_Request::STATUS;
  }

  I2CRfidSlave::~I2CRfidSlave() {}

  void I2CRfidSlave::I2CWrite(byte* data, uint8_t length) {
    for(int byte = 0; byte < length; byte++)
    {
      if(sizeof(data[byte]) != Wire.write(data[byte])) {
        Serial.print("ERROR: I2CWrite() write failed: return value ");
        Serial.println(data[byte]);
      }
    }
  }
  
  void I2CRfidSlave::receiveEvent(int num_bytes) {
    if(num_bytes == 1) {
      m_Request = Wire.read();
    }
    else {
      Serial.println("Incorrect message size");
    }
  }

  void I2CRfidSlave::requestEvent() {
    uint8_t reader_amount = m_RfidHandler.getReaderAmount();
    if(m_Request == I2C_Request::STATUS) {
      byte data[reader_amount];
      m_RfidHandler.readRfidState(data);
      I2CWrite(data, reader_amount);
    }
    else if(m_Request == I2C_Request::SENSOR_AMOUNT) {
      I2CWrite(&reader_amount, sizeof(reader_amount));
    }
    else if(m_Request == I2C_Request::CLEAR_UID_CACHE) {
      Serial.println("Clearing cache!");
      clearCache();
    }
    else {
      Serial.println("Unknown request");
    }
  }

  void I2CRfidSlave::addRfidReader(uint8_t ss_pin, uint8_t rst_pin, UID companion_tag = {{0, 0, 0, 0, 0, 0, 0, 0, 0, 0}}) {
    m_RfidHandler.addRfidReader(ss_pin, rst_pin, companion_tag);
  }

  void I2CRfidSlave::clearCache() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.clearCache();
    }
  }

  void I2CRfidSlave::read() {
    if(m_RfidHandler.getReaderAmount() > 0) {
      m_RfidHandler.read();
    }
  }
}