#!/usr/bin/python

import socket
from time import sleep
import sys
import datetime
import select

class stove:

    def __init__(self, IpStove,PortStove=2390): 
        self.ipStove = IpStove
        self.portStove = PortStove
        self.txSocket = None
        
    def connect(self):

        #if stove not connected         
        if self.txSocket is None:
            #Generate a transmit socket object
            self.txSocket = socket.socket(socket.AF_INET,socket.SOCK_DGRAM)
            
            #block when looking for received data
            self.txSocket.setblocking(1) 
            self.txSocket.settimeout(1)
        
    def get_value(self,memory,address):
        result = self.__send_received_read(memory,address)
        return {'memory':memory,'address':address, 'value':result , 'LSB':result & 0xff , 'MSB': result >> 8 }
        
        
    def switch_on(self):
        ''' switch on the stove
        
            parameters: 
        
            return:
                True if succeed false if fail
        '''        
        result = self.__send_received_write("80","E8",0x55)

        return result            
        
 
    def switch_off(self):
        ''' switch off the stove
        
            parameters: 
        
            return:
                True if succeed false if fail
        '''        
        result = self.__send_received_write("80","E8",0xAA)

        return result 


    def get_smoke_info(self):
        ''' get speed of motor that control pellet quantity
        
            parameters: 
        
            return:
                smokeRotorSpeed: in RPM
                smokeTemperature: in °c
        '''        
        smokeRotorSpeed = self.__send_received_read("0","2F")
        smokeTemperature = self.__send_received_read("0","2")

        return {'smokeRotorSpeed':smokeRotorSpeed*10, 'smokeTemperature':smokeTemperature}


    def get_pression_info(self):
        ''' get pression information of stove
        
            parameters: 
        
            return:
                pression: Pa
                pressionRequest: Pa
        '''        
        pression = self.__send_received_read("0","25")
        pressionRequest = self.__send_received_read("0","17")

        return {'pression':pression/10, 'pressionRequest':(pressionRequest&0xff)}


    def get_pellet_motor_info(self):
        ''' get speed of motor that control pellet quantity
        
            parameters: 
        
            return:
                pelletMotorSpeed: in RPM
                pelletMotorSpeedRequest: in RPM
                pelletMotorAmp: in mA
        '''        
        pelletMotorSpeed = self.__send_received_read("0","e")
        pelletMotorAmp = self.__send_received_read("0","4")
      
        return {'pelletMotorSpeed':(pelletMotorSpeed & 0xff),'pelletMotorSpeedRequest':(pelletMotorSpeed >> 8),'pelletMotorAmp':pelletMotorAmp}        
      
        
    def get_status(self):
        ''' get main temperature of stove
        
            return:
                dictionary with following temperature
                    setpoint : Actual temperature requested by user
                    ambiant : Actual room temperature
                    smoke : Actual smoke temperature
                    flame : Actual flame temperature
        '''        
        param8 = self.__send_received_read("0","8") & 0xF
        param10 = self.__send_received_read("0","10") & 0xF 
        param22 = self.__send_received_read("0","22") & 0xF
        param73 = self.__send_received_read("0","73") & 0xF
                    
        return {'param8':param8, 'param10':param10, 'param22':param22,'param73':param73}        
        
    def get_temperature(self):
        ''' get main temperature of stove
        
            return:
                dictionary with following temperature
                    setpoint : Actual temperature requested by user
                    ambiant : Actual room temperature
                    smoke : Actual smoke temperature
                    flame : Actual flame temperature
        '''        
        setpoint = self.__send_received_read("21","62")
        ambiant = self.__send_received_read("0","67")      
        flame = self.__send_received_read("0","0")       
        smoke  = self.__send_received_read("0","2")
        return {'setpoint':setpoint, 'ambiant':ambiant, 'smoke':smoke, 'flame':flame}


    def set_temperature(self, temperature):
        ''' get main temperature of stove
        
            parameters:
                temperature: temperature set 
        
            return:
                True if succeed false if fail
        '''        
        result = self.__send_received_write("A1","62",temperature)

        return result


    def get_power(self):
        ''' get actual power of stove
        
            return:
                dictionary with following temperature
                    power : Actual stove power 0 to 7

        '''        
        power = self.__send_received_read("21","60")
        return {'power':power}

    def set_power(self, power):
        ''' set power of stove
        
            parameters:
                power: from 0 to 7 
        
            return:
                True if succeed false if fail
        '''        
        result = self.__send_received_write("A1","60",power)

        return result

    def get_fan_speed(self):
        ''' get actual fan speed of stove
        
            return:
                dictionary with following temperature
                    fanSpeed : Actual stove fan speed 0 to 7

        '''        
        fanSpeed = self.__send_received_read("21","6C")
        return {'fanSpeed':fanSpeed}

    def set_fan_speed(self, fanSpeed):
        ''' set Fan speed of stove
        
            parameters:
                fanSpeed: fan Speed set 0 to 7
        
            return:
                True if succeed false if fail
        '''        
        result = self.__send_received_write("A1","6C",fanSpeed)

        return result    

    def get_date_time(self):
        ''' get actual date and time of stove
        
            return:
                dictionary with following temperature
                    stoveDT : date and time datetime type

        '''        
        hour = self.__send_received_read("25","0") & 0xff
        minute = self.__send_received_read("25","1") & 0xff
        day = self.__send_received_read("25","2") & 0xff
        month = self.__send_received_read("25","3") & 0xff
        year = (self.__send_received_read("25","4") & 0xff) + 2000
        #print(str(day) + "-" + str(month) + "-" + str(year) + " " + str(hour) + ":" + str(minute))
        stoveDT = datetime.datetime(year, month, day, hour=hour, minute=minute)
        return {'stoveDT':stoveDT}


    def set_date_time(self,dateTime):
        ''' get actual date and time of stove
                parameters:
                    dateTime: Date and time to be set (type datetime)
            return:
                True if succeed false if fail

        '''     
        value = dateTime.hour 
        result = self.__send_received_write("A5","0",value)
        if result == False:
            return  False        
        value = dateTime.minute
        result = self.__send_received_write("A5","1",value)
        if result == False:
            return  False   
        value = dateTime.day
        result = self.__send_received_write("A5","2",value)        
        if result == False:
            return  False   
        value = dateTime.month
        result = self.__send_received_write("A5","3",value)        
        if result == False:
            return  False   
        value = dateTime.year - 2000
        result = self.__send_received_write("A5","4",value)        
        if result == False:
            return  False   
        value = dateTime.isoweekday() 
        result = self.__send_received_write("A5","5",value) 
        if result == False:
            return  False   
        return True         


    def get_schedule(self,programmeNum):
        ''' get configuration of ChronoThermosta N
        
            parameters:
                programmeNum: 1 to 4
        
            return:
                dictionary with following parameters
                    startTime : time witch schedule start (HH:MM or none if not set)
                    endTime : time witch schedule end (HH:MM or none if not set)
                    dayOfWeek : witch day of week shcedule is set (bit0: Monday ... bit6:Sunday)
                    power : power of stove set (0 to 7)
                    temperature : temperature set 
                    fanSpeed : Fan speed set (0 to 7)
        '''        
        scheduleTab = [0,0x8C,0xA0,0xB4,0xC8]
        read = self.__send_received_read("21",'{:x}'.format((scheduleTab[programmeNum] + 0)))
        if read == 0x90:
            startTime = None
        else:
            startTime = '{:02d}H{:02d}'.format( (read // 6), (read % 6) * 10) 
            
        read = self.__send_received_read("21",'{:x}'.format((scheduleTab[programmeNum] + 2)))
        if read == 0x90:
            endTime = None
        else:
            endTime = '{:02d}H{:02d}'.format( (read // 6), (read % 6) * 10) 
            
        dayOfWeek = self.__send_received_read("21",'{:x}'.format((scheduleTab[programmeNum] + 4))) 
        
        power = self.__send_received_read("21",'{:x}'.format((scheduleTab[programmeNum] + 6)))                          

        temperature = self.__send_received_read("21",'{:x}'.format((scheduleTab[programmeNum] + 8))) 

        fanSpeed = self.__send_received_read("21",'{:x}'.format((scheduleTab[programmeNum] + 14))) 
                
        return {'startTime':startTime, 'endTime':endTime, 'dayOfWeek':dayOfWeek, 'power':power, 'temperature':temperature, 'fanSpeed':fanSpeed}


    def set_schedule(self,programmeNum,startTime,endTime,dayOfWeek,power,temperature,fanSpeed):
        ''' get configuration of ChronoThermosta N
        
            parameters:
                programmeNum: 1 to 4
                startTime : time witch schedule start (HH:MM or none if not set)
                endTime : time witch schedule end (HH:MM or none if not set)
                dayOfWeek : witch day of week shcedule is set (bit0: Monday ... bit6:Sunday)
                power : power of stove set (0 to 7)
                temperature : temperature set 
                fanSpeed : Fan speed set (0 to 7)        
            return:
                True if succeed false if fail
        '''        
        scheduleTab = [0,0x8C,0xA0,0xB4,0xC8]
        
        if startTime is None:
            value = 0x90
        else:
            HMvar = startTime.upper().split('H')
            value = int(HMvar[0])*6 + ((int(HMvar[1])//10)) 
        res = self.__send_received_write("A1",'{:x}'.format(scheduleTab[programmeNum] + 0),value)
        if res == False:
            return False

        if endTime is None:
            value  = 0x90
        else:
            HMvar = endTime.upper().split('H')
            value = int(HMvar[0])*6 + ((int(HMvar[1])//10))
        res = self.__send_received_write("A1",'{:x}'.format(scheduleTab[programmeNum] + 2),value)
        if res == False:
            return  False
            
        res = self.__send_received_write("A1",'{:x}'.format(scheduleTab[programmeNum] + 4),dayOfWeek) 
        if res == False:
            return  False
                    
        res = self.__send_received_write("A1",'{:x}'.format(scheduleTab[programmeNum] + 6),power)                          
        if res == False:
            return  False
            
        res = self.__send_received_write("A1",'{:x}'.format(scheduleTab[programmeNum] + 8),temperature) 
        if res == False:
            return  False
            
        res = self.__send_received_write("A1",'{:x}'.format(scheduleTab[programmeNum] + 14),fanSpeed) 
        if res == False:
            return False
                            
        return True

        
    def __send_received_read(self,memory,adresse):
        ''' Ask stove memmory value
        
            parameters:
                memory : 0 , 21 or 25
                adresse : adresse read requested
                
            return :
                value : returned read value or None if fail
        '''
        if self.txSocket is None:
            return None

        #empty socket before send command
        try:
            data, addr = self.txSocket.recvfrom(1024)
        except:
            pass
            
        txChar = str(memory) + ";" + str(adresse) + "\n"
        #print(txChar)
        #Transmit data to the stove on the agreed-upon port
        self.txSocket.sendto(txChar.encode(),(self.ipStove,self.portStove))
        
        
        #Attempt to receive the echo from the server
        data, addr = self.txSocket.recvfrom(20)
        print(data)
        if int(data.decode()) != -1:
            return int(data.decode())

        return None 
                        
    
    def __send_received_write(self,memory,adresse,value):
        ''' Ask stove memmory value
        
            parameters:
                memory : 0 , 21 or 25
                adresse : adresse read requested
                
            return :
                value : returned read value or None if fail
        '''
        if self.txSocket is None:
            return None
            
        txChar = str(memory) + ";" + str(adresse) + ";" + '{:x}'.format(value) + "\n"
        #print(txChar)
        #Transmit data to the stove on the agreed-upon port
        self.txSocket.sendto(txChar.encode(),(self.ipStove,self.portStove))
        
        
        #Attempt to receive the echo from the server
        data, addr = self.txSocket.recvfrom(30)
        #print(data.decode())
        if "successfully" in data.decode():
            return True
        else:
            return False
          
    


if __name__=="__main__":

    AC10 = stove("192.168.25.105")
    
    AC10.connect()
    #sleep(2)
    
    #Temperature = AC10.get_temperature()
    #print(Temperature)
    
    #Programme1 = AC10.get_schedule(1)
    #print(Programme1)
    
    #Programme2 = AC10.get_schedule(2)
    #print(Programme2)
    
    #Programme3 = AC10.get_schedule(3)
    #print(Programme3)
    
    #Programme4 = AC10.get_schedule(4)
    #print(Programme4)   
    
    #res = AC10.set_temperature(24)
    #print(res)
    
    #Temperature = AC10.get_temperature()
    #print(Temperature)    
     
    #res = AC10.set_schedule(programmeNum=1,startTime="5H00",endTime="7H00",dayOfWeek=int('1111100',2),power=4,temperature=22,fanSpeed=4)
    #res = AC10.set_schedule(programmeNum=2,startTime=None,endTime=None,dayOfWeek=int('1111100',2),power=4,temperature=22,fanSpeed=4)    
    #res = AC10.set_schedule(programmeNum=3,startTime="17H00",endTime="22H00",dayOfWeek=int('1111100',2),power=4,temperature=22,fanSpeed=4)
    #print(res)
    
    #Programme1 = AC10.get_schedule(1)
    #print(Programme1) 
    
    #datestove = AC10.get_date_time()  
    #print(datestove['stoveDT'])
    
    #res = AC10.set_date_time(datetime.datetime.today())  
    #print(res)  
     
    #res = AC10.switch_on()
    #print(res) 
    
    #res = AC10.get_status()
    #print(res)
    
    #res = AC10.get_pellet_motor_info()
    #print(res)    
    
    #res = AC10.get_pression_info()
    #print(res)     
    
    #for param in range(0,130,2):
    #    print(AC10.get_value("0",'{0:x}'.format(param)))
    
    #for param in ['4','c','14','2a','2e','30','34','3a','40','42','60','62','6c','78','7c']:
    #for param in ['60','6c','7c']:
    #    try:
    #        print(AC10.get_value("0",param))
    #    except:
    #        pass
