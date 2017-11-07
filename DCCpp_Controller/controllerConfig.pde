//////////////////////////////////////////////////////////////////////////
//  DCC++ CONTROLLER: Configuration and Initialization
//
//  * Defines all global variables and objects
//
//  * Reads and loads previous status data from status files
//
//  * Implements track layout(s), throttles, track buttons, route buttons,
//    cab buttons, function buttons, windows, current meter,
//    and all other user-specified components
//
//////////////////////////////////////////////////////////////////////////

// DECLARE "GLOBAL" VARIABLES and OBJECTS

  PApplet Applet = this;                         // Refers to this program --- needed for Serial class

  int cursorType;
  String baseID;
  boolean keyHold=false;
  boolean saveXMLFlag=false;
  int lastTime;
  PFont throttleFont, messageFont, buttonFont;
  color backgroundColor;
  XML dccStatusXML, arduinoPortXML, sensorButtonsXML, autoPilotXML, cabDefaultsXML, serverListXML;
  
  DccComponent selectedComponent, previousComponent;
  ArrayList<DccComponent> dccComponents = new ArrayList<DccComponent>();
  ArrayList<CabButton> cabButtons = new ArrayList<CabButton>();
  ArrayList<CallBack> callBacks = new ArrayList<CallBack>();
  ArrayList<DccComponent> buttonQueue = new ArrayList<DccComponent>();
  ArrayList<DccComponent> buttonQueue2 = new ArrayList<DccComponent>();
  HashMap<Integer,EllipseButton> remoteButtonsHM = new HashMap<Integer,EllipseButton>();
  ArrayList<MessageBox> msgAutoCab = new ArrayList<MessageBox>();
  HashMap<Integer,TrackSensor> sensorsHM = new HashMap<Integer,TrackSensor>();    
  HashMap<String,CabButton> cabsHM = new HashMap<String,CabButton>();
  HashMap<Integer,TrackButton> trackButtonsHM = new HashMap<Integer,TrackButton>();  
  
  ArduinoPort       aPort;
  PowerButton       powerButton;
  AutoPilotButton   autoPilot;
  CleaningCarButton cleaningCab;
  Throttle          throttleA;
  Layout            layout,layout2,layoutBridge;
  MessageBox        msgBoxMain, msgBoxDiagIn, msgBoxDiagOut, msgBoxClock;
  CurrentMeter      currentMeter;
  Window            mainWindow, accWindow, progWindow, portWindow, extrasWindow, opWindow, diagWindow, autoWindow, sensorWindow, ledWindow;
  ImageWindow       imageWindow;
  JPGWindow         helpWindow;
  MessageBox        msgAutoState, msgAutoTimer;
  InputBox          activeInputBox;
  InputBox          accAddInput, accSubAddInput;
  InputBox          progCVInput, progHEXInput, progDECInput, progBINInput;
  InputBox          opCabInput, opCVInput, opHEXInput, opDECInput, opBINInput, opBitInput;
  InputBox          shortAddInput, longAddInput;
  MessageBox        activeAddBox;
  MessageBox        portBox, portNumBox;
  MessageBox        ledHueMsg, ledSatMsg, ledValMsg, ledRedMsg, ledGreenMsg, ledBlueMsg;
  PortScanButton    portScanButton;
  LEDColorButton    ledColorButton;
  
// DECLARE TRACK BUTTONS, ROUTE BUTTONS, and CAB BUTTONS WHICH WILL BE DEFINED BELOW AND USED "GLOBALLY"  

  TrackButton      tButton1,tButton2,tButton3,tButton4,tButton5;
  TrackButton      tButton6,tButton7,tButton8,tButton9,tButton10;
  TrackButton      tButton20,tButton30,tButton40,tButton50;
  
  RouteButton      rButton1,rButton2,rButton3,rButton4,rButton5,rButton6,rButton7;
  RouteButton      rButton10,rButton11,rButton12,rButton13,rButton14;
  RouteButton      rButtonR1,rButtonR2,rButton15,rButton16,rButton17,rButtonSpiral,rButtonReset,rButtonBridge;  

  CabButton        cab8601,cab54,cab1202,cab1506,cab622,cab2004,cab6021;
  
////////////////////////////////////////////////////////////////////////
//  Initialize --- configures everything!
////////////////////////////////////////////////////////////////////////

  void Initialize(){
    colorMode(RGB,255);
    throttleFont=loadFont("OCRAExtended-26.vlw");
    messageFont=loadFont("LucidaConsole-18.vlw");
    buttonFont=loadFont("LucidaConsole-18.vlw");
    rectMode(CENTER);
    textAlign(CENTER,CENTER);
    backgroundColor=color(50,50,60);

    aPort=new ArduinoPort();
    
// READ, OR CREATE IF NEEDED, XML DCC STATUS FILE
    
    dccStatusXML=loadXML(STATUS_FILE);
    if(dccStatusXML==null){
      dccStatusXML=new XML("dccStatus");
    }

    arduinoPortXML=dccStatusXML.getChild("arduinoPort");
    if(arduinoPortXML==null){
      arduinoPortXML=dccStatusXML.addChild("arduinoPort");
      arduinoPortXML.setContent("Emulator");
    }
    
    serverListXML=dccStatusXML.getChild("serverList");
    if(serverListXML==null){
      serverListXML=dccStatusXML.addChild("serverList");
      serverListXML.setContent("127.0.0.1");
    }
    
    sensorButtonsXML=dccStatusXML.getChild("sensorButtons");
    if(sensorButtonsXML==null){
      sensorButtonsXML=dccStatusXML.addChild("sensorButtons");
    }

    autoPilotXML=dccStatusXML.getChild("autoPilot");
    if(autoPilotXML==null){
      autoPilotXML=dccStatusXML.addChild("autoPilot");
    }
    
    cabDefaultsXML=dccStatusXML.getChild("cabDefaults");
    if(cabDefaultsXML==null){
      cabDefaultsXML=dccStatusXML.addChild("cabDefaults");
    }
    
    saveXMLFlag=true;
      
// CREATE THE ACCESSORY CONTROL WINDOW
    
    accWindow = new Window(292,544,280,155,color(200,200,200),color(200,50,50));
    new DragBar(accWindow,0,0,280,10,color(200,50,50));
    new CloseButton(accWindow,268,0,10,10,color(200,50,50),color(255,255,255));
    new MessageBox(accWindow,150,22,0,0,color(200,200,200),20,"Accessory Control",color(200,50,50));
    new MessageBox(accWindow,20,60,-1,0,color(200,200,200),16,"Acc Address (0-511):",color(200,50,50));
    accAddInput = new InputBox(accWindow,230,60,16,color(200,200,200),color(50,50,200),3,InputType.DEC);
    new MessageBox(accWindow,20,90,-1,0,color(200,200,200),16,"Sub Address   (0-3):",color(200,50,50));
    accSubAddInput = new InputBox(accWindow,230,90,16,color(200,200,200),color(50,50,200),1,InputType.DEC);
    new AccessoryButton(accWindow,90,130,55,25,100,18,"ON",accAddInput,accSubAddInput);
    new AccessoryButton(accWindow,210,130,55,25,0,18,"OFF",accAddInput,accSubAddInput);
    accAddInput.setNextBox(accSubAddInput);
    accSubAddInput.setNextBox(accAddInput);
    
// CREATE THE SERIAL PORT WINDOW
    
    portWindow = new Window(100,200,500,170,color(200,200,200),color(200,50,50));
    new DragBar(portWindow,0,0,500,10,color(200,50,50));
    new CloseButton(portWindow,488,0,10,10,color(200,50,50),color(255,255,255));
    new MessageBox(portWindow,250,22,0,0,color(200,200,200),20,"Select Arduino Port",color(200,50,50));
    portScanButton = new PortScanButton(portWindow,100,60,85,20,100,18,"SCAN");
    new PortScanButton(portWindow,400,60,85,20,0,18,"CONNECT");
    new PortScanButton(portWindow,120,140,15,20,120,18,"<");
    new PortScanButton(portWindow,380,140,15,20,120,18,">");
    portBox = new MessageBox(portWindow,250,100,380,25,color(250,250,250),20,"",color(50,150,50));
    portBox.setMessage("Please press SCAN",color(150,50,50));
    portNumBox = new MessageBox(portWindow,250,140,0,0,color(200,200,200),20,"",color(50,50,50));

// CREATE THE PROGRAMMING CVs ON THE PROGRAMMING TRACK WINDOW
    
    progWindow = new Window(100,100,500,400,color(200,180,200),color(50,50,200));
    new DragBar(progWindow,0,0,500,10,color(50,50,200));
    new CloseButton(progWindow,488,0,10,10,color(50,50,200),color(255,255,255));
    new RectButton(progWindow,250,30,210,30,40,color(0),18,"Programming Track",ButtonType.TI_COMMAND,101);        
    
    new MessageBox(progWindow,20,90,-1,0,color(200,180,200),16,"CV (1-1024):",color(50,50,200));
    new MessageBox(progWindow,20,130,-1,0,color(200,180,200),16,"Value (HEX):",color(50,50,200));
    new MessageBox(progWindow,20,160,-1,0,color(200,180,200),16,"Value (DEC):",color(50,50,200));
    new MessageBox(progWindow,20,190,-1,0,color(200,180,200),16,"Value (BIN):",color(50,50,200));
    progCVInput = new InputBox(progWindow,150,90,16,color(200,180,200),color(200,50,50),4,InputType.DEC);
    progHEXInput = new InputBox(progWindow,150,130,16,color(200,180,200),color(200,50,50),2,InputType.HEX);
    progDECInput = new InputBox(progWindow,150,160,16,color(200,180,200),color(200,50,50),3,InputType.DEC);
    progBINInput = new InputBox(progWindow,150,190,16,color(200,180,200),color(200,50,50),8,InputType.BIN);
    progCVInput.setNextBox(progHEXInput);
    progHEXInput.setNextBox(progDECInput);
    progDECInput.setNextBox(progBINInput);
    progDECInput.linkBox(progHEXInput);
    progBINInput.setNextBox(progHEXInput);
    progBINInput.linkBox(progHEXInput);        
    new ProgWriteReadButton(progWindow,300,90,65,25,100,14,"READ",progCVInput,progHEXInput);
    new ProgWriteReadButton(progWindow,390,90,65,25,0,14,"WRITE",progCVInput,progHEXInput);

    new MessageBox(progWindow,20,240,-1,0,color(200,180,200),16,"ENGINE ADDRESSES",color(50,50,200));
    new MessageBox(progWindow,20,280,-1,0,color(200,180,200),16,"Short  (1-127):",color(50,50,200));
    new MessageBox(progWindow,20,310,-1,0,color(200,180,200),16,"Long (0-10239):",color(50,50,200));
    new MessageBox(progWindow,20,340,-1,0,color(200,180,200),16,"Active        :",color(50,50,200));
    shortAddInput = new InputBox(progWindow,190,280,16,color(200,180,200),color(200,50,50),3,InputType.DEC);
    longAddInput = new InputBox(progWindow,190,310,16,color(200,180,200),color(200,50,50),5,InputType.DEC);
    activeAddBox = new MessageBox(progWindow,190,340,-1,0,color(200,180,200),16,"?",color(200,50,50));
    new ProgAddReadButton(progWindow,300,240,65,25,100,14,"READ",shortAddInput,longAddInput,activeAddBox);
    new ProgShortAddWriteButton(progWindow,300,280,65,25,0,14,"WRITE",shortAddInput);
    new ProgLongAddWriteButton(progWindow,300,310,65,25,0,14,"WRITE",longAddInput);
    new ProgLongShortButton(progWindow,300,340,65,25,0,14,"Long",activeAddBox);
    new ProgLongShortButton(progWindow,390,340,65,25,0,14,"Short",activeAddBox);

// CREATE THE PROGRAMMING CVs ON THE MAIN OPERATIONS TRACK WINDOW
    
    opWindow = new Window(100,100,500,300,color(220,200,200),color(50,50,200));
    new DragBar(opWindow,0,0,500,10,color(50,50,200));
    new CloseButton(opWindow,488,0,10,10,color(50,50,200),color(255,255,255));
    new MessageBox(opWindow,250,30,0,0,color(220,200,200),20,"Operations Programming",color(50,100,50));
    new MessageBox(opWindow,20,90,-1,0,color(220,200,200),16,"Cab Number :",color(50,50,200));
    new MessageBox(opWindow,20,120,-1,0,color(220,200,200),16,"CV (1-1024):",color(50,50,200));
    new MessageBox(opWindow,20,160,-1,0,color(220,200,200),16,"Value (HEX):",color(50,50,200));
    new MessageBox(opWindow,20,190,-1,0,color(220,200,200),16,"Value (DEC):",color(50,50,200));
    new MessageBox(opWindow,20,220,-1,0,color(220,200,200),16,"Value (BIN):",color(50,50,200));
    opCabInput = new InputBox(opWindow,150,90,16,color(220,200,200),color(200,50,50),5,InputType.DEC);
    opCVInput = new InputBox(opWindow,150,120,16,color(220,200,200),color(200,50,50),4,InputType.DEC);
    opHEXInput = new InputBox(opWindow,150,160,16,color(220,200,200),color(200,50,50),2,InputType.HEX);
    opDECInput = new InputBox(opWindow,150,190,16,color(220,200,200),color(200,50,50),3,InputType.DEC);
    opBINInput = new InputBox(opWindow,150,220,16,color(220,200,200),color(200,50,50),8,InputType.BIN);
    opCVInput.setNextBox(opHEXInput);
    opHEXInput.setNextBox(opDECInput);
    opDECInput.setNextBox(opBINInput);
    opDECInput.linkBox(opHEXInput);
    opBINInput.setNextBox(opHEXInput);
    opBINInput.linkBox(opHEXInput);        
    new OpWriteButton(opWindow,300,90,65,25,0,14,"WRITE",opCVInput,opHEXInput);
    new MessageBox(opWindow,20,260,-1,0,color(220,200,200),16,"  Bit (0-7):",color(50,50,200));
    opBitInput = new InputBox(opWindow,150,260,16,color(220,200,200),color(200,50,50),1,InputType.DEC);
    new OpWriteButton(opWindow,300,260,65,25,50,14,"SET",opCVInput,opBitInput);
    new OpWriteButton(opWindow,390,260,65,25,150,14,"CLEAR",opCVInput,opBitInput);

// CREATE THE DCC++ CONTROL <-> DCC++ BASE STATION COMMUNICATION DIAGNOSTICS WINDOW
    
    diagWindow = new Window(100,300,500,120,color(175),color(50,200,50));
    new DragBar(diagWindow,0,0,500,10,color(50,200,50));
    new CloseButton(diagWindow,488,0,10,10,color(50,200,50),color(255,255,255));
    new MessageBox(diagWindow,250,20,0,0,color(175),18,"Diagnostics Window",color(50,50,200));
    new MessageBox(diagWindow,10,60,-1,0,color(175),18,"Sent:",color(50,50,200));
    msgBoxDiagOut=new MessageBox(diagWindow,250,60,0,0,color(175),18,"---",color(50,50,200));
    new MessageBox(diagWindow,10,90,-1,0,color(175),18,"Proc:",color(50,50,200));
    msgBoxDiagIn=new MessageBox(diagWindow,250,90,0,0,color(175),18,"---",color(50,50,200));

// CREATE THE AUTOPILOT DIAGNOSTICS WINDOW 
    
    autoWindow = new Window(100,300,500,330,color(175),color(50,200,50));
    new DragBar(autoWindow,0,0,500,10,color(50,200,50));
    new CloseButton(autoWindow,488,0,10,10,color(50,200,50),color(255,255,255));
    new MessageBox(autoWindow,250,20,0,0,color(175),18,"AutoPilot Window",color(50,50,150));
    msgAutoState=new MessageBox(autoWindow,0,180,-1,0,color(175),18,"?",color(50,50,250));
    msgAutoTimer=new MessageBox(autoWindow,55,310,-1,0,color(175),18,"Timer =",color(50,50,250));
    
// CREATE THE SENSORS DIAGNOSTICS WINDOW 
    
    sensorWindow = new Window(100,300,500,350,color(175),color(50,200,50));
    new DragBar(sensorWindow,0,0,500,10,color(50,200,50));
    new CloseButton(sensorWindow,488,0,10,10,color(50,200,50),color(255,255,255));
    new MessageBox(sensorWindow,250,20,0,0,color(175),18,"Sensors Window",color(50,50,150));

// CREATE THE HELP WINDOW
      
  helpWindow=new JPGWindow("helpMenu.jpg",220,457,320,60,color(0,100,0));    
        
// CREATE THE EXTRAS WINDOW:

    extrasWindow = new Window(100,200,500,250,color(255,255,175),color(100,100,200));
    new DragBar(extrasWindow,0,0,500,10,color(100,100,200));
    new CloseButton(extrasWindow,488,0,10,10,color(100,100,200),color(255,255,255));
    new MessageBox(extrasWindow,250,20,0,0,color(175),18,"Extra Functions",color(50,50,200));
//    new RectButton(extrasWindow,260,80,120,50,85,color(0),16,"Sound\nEffects",0);        

// CREATE THE LED LIGHT-STRIP WINDOW:

    ledWindow = new Window(100,200,550,425,color(0),color(0,0,200));
    new DragBar(ledWindow,0,0,550,10,color(0,0,200));
    new CloseButton(ledWindow,538,0,10,10,color(0,0,200),color(200,200,200));
    new MessageBox(ledWindow,275,20,0,0,color(175),18,"LED Light Strip",color(200,200,200));
    ledColorButton=new LEDColorButton(ledWindow,310,175,30,201,0.0,0.0,1.0);
    new LEDColorSelector(ledWindow,150,175,100,ledColorButton);
    new LEDValSelector(ledWindow,50,330,200,30,ledColorButton);
    ledHueMsg = new MessageBox(ledWindow,360,80,-1,0,color(175),18,"Hue:   -",color(200,200,200));
    ledSatMsg = new MessageBox(ledWindow,360,115,-1,0,color(175),18,"Sat:   -",color(200,200,200));
    ledValMsg = new MessageBox(ledWindow,360,150,-1,0,color(175),18,"Val:   -",color(200,200,200));
    ledRedMsg = new MessageBox(ledWindow,360,185,-1,0,color(175),18,"Red:   -",color(200,200,200));
    ledGreenMsg = new MessageBox(ledWindow,360,220,-1,0,color(175),18,"Green: -",color(200,200,200));
    ledBlueMsg = new MessageBox(ledWindow,360,255,-1,0,color(175),18,"Blue:  -",color(200,200,200));

// CREATE TOP-OF-SCREEN MESSAGE BAR AND HELP BUTTON

    msgBoxMain=new MessageBox(width/2,12,width,25,color(200),20,"Searching for Base Station: "+arduinoPortXML.getContent(),color(30,30,150));
    new HelpButton(width-50,12,22,22,150,20,"?");

// CREATE CLOCK

    msgBoxClock=new MessageBox(30,700,-100,30,backgroundColor,30,"00:00:00",color(255,255,255));
    
// CREATE POWER BUTTON, QUIT BUTTON, and CURRENT METER
    
    powerButton=new PowerButton(75,475,100,30,100,18,"POWER");
    new QuitButton(75,510,100,30,250,18,"QUIT");
    currentMeter = new CurrentMeter(25,550,150,100,675,5);

// CREATE THROTTLE, DEFINE CAB BUTTONS, and SET FUNCTIONS FOR EACH CAB
    
    int tAx=175;
    int tAy=225;
    int rX=800;
    int rY=550;

    throttleA=new Throttle(tAx,tAy,1.3);
    
    cab2004 = new CabButton(tAx-125,tAy-150,50,30,150,15,3,throttleA);
    cab2004.setThrottleDefaults(100,50,-50,-45);
    cab2004.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab2004.setFunction(35,15,60,22,60,10,0,"F0",ButtonType.NORMAL);
    cab2004.setFunction(35,45,60,22,60,10,1,"F1",ButtonType.NORMAL);
    cab2004.setFunction(35,75,60,22,60,10,2,"F2",ButtonType.NORMAL);
    cab2004.setFunction(35,105,60,22,60,10,3,"F3",ButtonType.NORMAL);
    cab2004.setFunction(35,135,60,22,60,10,4,"F4",ButtonType.NORMAL);
    cab2004.setFunction(35,165,60,22,60,10,5,"F5",ButtonType.NORMAL);
    cab2004.setFunction(35,195,60,22,60,10,6,"F6",ButtonType.NORMAL);
    cab2004.setFunction(35,225,60,22,60,10,7,"F7",ButtonType.NORMAL);
    cab2004.setFunction(35,255,60,22,60,10,8,"F8",ButtonType.NORMAL);
    cab2004.setFunction(35,285,60,22,60,10,9,"F9",ButtonType.NORMAL);
    cab2004.setFunction(35,315,60,22,60,10,10,"F10",ButtonType.NORMAL);
    cab2004.setFunction(35,345,60,22,60,10,11,"F11",ButtonType.NORMAL);
    cab2004.setFunction(35,375,60,22,60,10,12,"F12",ButtonType.NORMAL);
    cab2004.setFunction(35,405,60,22,60,10,13,"F13",ButtonType.NORMAL);
    cab2004.setFunction(35,435,60,22,60,10,14,"F14",ButtonType.NORMAL);
    cab2004.setFunction(35,465,60,22,60,10,15,"F15",ButtonType.NORMAL);
    cab2004.setFunction(35,495,60,22,60,10,16,"F16",ButtonType.NORMAL);
    cab2004.setFunction(35,525,60,22,60,10,17,"F17",ButtonType.NORMAL);
    
    cab622 = new CabButton(tAx-125,tAy-100,50,30,150,15,24,throttleA);
    cab622.setThrottleDefaults(53,30,-20,-13);
    cab622.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab622.setFunction(35,15,60,22,60,10,0,"Lights",ButtonType.NORMAL,CabFunction.F_LIGHT,CabFunction.R_LIGHT);
    cab622.setFunction(35,45,60,22,60,10,1,"Bell",ButtonType.NORMAL,CabFunction.BELL);
    cab622.setFunction(35,75,60,22,60,10,2,"Horn",ButtonType.HOLD,CabFunction.HORN);
    cab622.setFunction(35,105,60,22,60,10,3,"F3",ButtonType.NORMAL);
    cab622.setFunction(35,135,60,22,60,10,4,"F4",ButtonType.NORMAL);
    cab622.setFunction(35,165,60,22,60,10,5,"F5",ButtonType.NORMAL);
    cab622.setFunction(35,195,60,22,60,10,6,"F6",ButtonType.NORMAL);
    cab622.setFunction(35,225,60,22,60,10,7,"F7",ButtonType.NORMAL);
    cab622.setFunction(35,255,60,22,60,10,8,"F8",ButtonType.NORMAL);
    cab622.setFunction(35,285,60,22,60,10,9,"F9",ButtonType.NORMAL);
    cab622.setFunction(35,315,60,22,60,10,10,"F10",ButtonType.NORMAL);
    cab622.setFunction(35,345,60,22,60,10,11,"F11",ButtonType.NORMAL);
    cab622.setFunction(35,375,60,22,60,10,12,"F12",ButtonType.NORMAL);
    cab622.setFunction(35,405,60,22,60,10,13,"F13",ButtonType.NORMAL);
    cab622.setFunction(35,435,60,22,60,10,14,"F14",ButtonType.NORMAL);
    cab622.setFunction(35,465,60,22,60,10,15,"F15",ButtonType.NORMAL);
    cab622.setFunction(35,495,60,22,60,10,16,"F16",ButtonType.NORMAL);
    cab622.setFunction(35,525,60,22,60,10,17,"F17",ButtonType.NORMAL);

    cab8601 = new CabButton(tAx-125,tAy-50,50,30,150,15,27,throttleA);
    cab8601.setThrottleDefaults(77,46,-34,-30);
    cab8601.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab8601.setFunction(35,15,60,22,60,10,0,"F0",ButtonType.NORMAL);
    cab8601.setFunction(35,45,60,22,60,10,1,"F1",ButtonType.NORMAL);
    cab8601.setFunction(35,75,60,22,60,10,2,"F2",ButtonType.NORMAL);
    cab8601.setFunction(35,105,60,22,60,10,3,"F3",ButtonType.NORMAL);
    cab8601.setFunction(35,135,60,22,60,10,4,"F4",ButtonType.NORMAL);
    cab8601.setFunction(35,165,60,22,60,10,5,"F5",ButtonType.NORMAL);
    cab8601.setFunction(35,195,60,22,60,10,6,"F6",ButtonType.NORMAL);
    cab8601.setFunction(35,225,60,22,60,10,7,"F7",ButtonType.NORMAL);
    cab8601.setFunction(35,255,60,22,60,10,8,"F8",ButtonType.NORMAL);
    cab8601.setFunction(35,285,60,22,60,10,9,"F9",ButtonType.NORMAL);
    cab8601.setFunction(35,315,60,22,60,10,10,"F10",ButtonType.NORMAL);
    cab8601.setFunction(35,345,60,22,60,10,11,"F11",ButtonType.NORMAL);
    cab8601.setFunction(35,375,60,22,60,10,12,"F12",ButtonType.NORMAL);
    cab8601.setFunction(35,405,60,22,60,10,13,"F13",ButtonType.NORMAL);
    cab8601.setFunction(35,435,60,22,60,10,14,"F14",ButtonType.NORMAL);
    cab8601.setFunction(35,465,60,22,60,10,15,"F15",ButtonType.NORMAL);
    cab8601.setFunction(35,495,60,22,60,10,16,"F16",ButtonType.NORMAL);
    cab8601.setFunction(35,525,60,22,60,10,17,"F17",ButtonType.NORMAL);

    cab6021 = new CabButton(tAx-125,tAy,50,30,150,15,6021,throttleA);
    cab6021.setThrottleDefaults(50,25,-25,-15);
    cab6021.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab6021.setFunction(35,15,60,22,60,10,0,"Headlight",ButtonType.NORMAL,CabFunction.F_LIGHT);
    cab6021.setFunction(35,45,60,22,60,10,1,"Tailight",ButtonType.NORMAL,CabFunction.R_LIGHT);

    cab54 = new CabButton(tAx-125,tAy+50,50,30,150,15,54,throttleA);
    cab54.setThrottleDefaults(34,14,-5,-3);
    cab54.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab54.setFunction(35,15,60,22,60,10,10,"Radiator\nFan",ButtonType.NORMAL);
    cab54.setFunction(35,45,60,22,60,10,11,"Air Fill\n/Release",ButtonType.ONESHOT);
    cab54.setFunction(35,75,60,22,60,10,14,"Passenger\nDep/Arr",ButtonType.ONESHOT);
    cab54.setFunction(35,105,60,22,60,10,18,"City\nSounds",ButtonType.ONESHOT);
    cab54.setFunction(35,135,60,22,60,10,19,"Farm\nSounds",ButtonType.ONESHOT);
    cab54.setFunction(35,165,60,22,60,10,21,"Lumber\nMill",ButtonType.ONESHOT);
    cab54.setFunction(35,195,60,22,60,10,20,"Industry\nSounds",ButtonType.ONESHOT);
    cab54.setFunction(35,225,60,22,60,10,13,"Crossing\nHorn",ButtonType.ONESHOT,CabFunction.S_HORN);
    cab54.setFunction(35,255,60,22,60,10,22,"Alternate\nHorn",ButtonType.NORMAL);
    cab54.setFunction(35,285,60,22,60,10,8,"Mute",ButtonType.NORMAL);
    cab54.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab54.setFunction(35,15,60,22,60,10,0,"Headlight",ButtonType.NORMAL,CabFunction.F_LIGHT);
    cab54.setFunction(35,45,60,22,60,10,1,"Bell",ButtonType.NORMAL,CabFunction.BELL);
    cab54.setFunction(35,75,60,22,60,10,2,"Horn",ButtonType.HOLD,CabFunction.HORN);
    cab54.setFunction(35,105,60,22,60,10,3,"MARS\nLight",ButtonType.REVERSE,CabFunction.D_LIGHT);
    cab54.setFunction(35,135,16,22,60,10,9,"1",ButtonType.NORMAL);
    cab54.setFunction(14,135,16,22,60,10,5,"+",ButtonType.ONESHOT);
    cab54.setFunction(56,135,16,22,60,10,6,"-",ButtonType.ONESHOT);
    cab54.setFunction(35,165,60,22,60,10,15,"Freight\nDep/Arr",ButtonType.ONESHOT);
    cab54.setFunction(35,195,60,22,60,10,16,"Facility\nShop",ButtonType.ONESHOT);
    cab54.setFunction(35,225,60,22,60,10,17,"Crew\nRadio",ButtonType.ONESHOT);
    cab54.setFunction(35,255,60,22,60,10,7,"Coupler",ButtonType.ONESHOT);
    cab54.setFunction(35,285,60,22,60,10,4,"Dynamic\nBrake",ButtonType.NORMAL);
    cab54.setFunction(35,315,60,22,60,10,12,"Brake\nSqueal",ButtonType.ONESHOT);

    cab1202 = new CabButton(tAx-125,tAy+100,50,30,150,15,1202,throttleA);
    cab1202.setThrottleDefaults(34,25,-24,-18);
    cab1202.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab1202.setFunction(35,15,60,22,60,10,0,"F0",ButtonType.NORMAL);
    cab1202.setFunction(35,45,60,22,60,10,1,"F1",ButtonType.NORMAL);
    cab1202.setFunction(35,75,60,22,60,10,2,"F2",ButtonType.NORMAL);
    cab1202.setFunction(35,105,60,22,60,10,3,"F3",ButtonType.NORMAL);
    cab1202.setFunction(35,135,60,22,60,10,4,"F4",ButtonType.NORMAL);
    cab1202.setFunction(35,165,60,22,60,10,5,"F5",ButtonType.NORMAL);
    cab1202.setFunction(35,195,60,22,60,10,6,"F6",ButtonType.NORMAL);
    cab1202.setFunction(35,225,60,22,60,10,7,"F7",ButtonType.NORMAL);
    cab1202.setFunction(35,255,60,22,60,10,8,"F8",ButtonType.NORMAL);
    cab1202.setFunction(35,285,60,22,60,10,9,"F9",ButtonType.NORMAL);
    cab1202.setFunction(35,315,60,22,60,10,10,"F10",ButtonType.NORMAL);
    cab1202.setFunction(35,345,60,22,60,10,11,"F11",ButtonType.NORMAL);
    cab1202.setFunction(35,375,60,22,60,10,12,"F12",ButtonType.NORMAL);
    cab1202.setFunction(35,405,60,22,60,10,13,"F13",ButtonType.NORMAL);
    cab1202.setFunction(35,435,60,22,60,10,14,"F14",ButtonType.NORMAL);
    cab1202.setFunction(35,465,60,22,60,10,15,"F15",ButtonType.NORMAL);
    cab1202.setFunction(35,495,60,22,60,10,16,"F16",ButtonType.NORMAL);
    cab1202.setFunction(35,525,60,22,60,10,17,"F17",ButtonType.NORMAL);

    cab1506 = new CabButton(tAx-125,tAy+150,50,30,150,15,1506,throttleA);
    cab1506.setThrottleDefaults(61,42,-30,-22);    
    cab1506.functionButtonWindow(220,59,70,340,backgroundColor,backgroundColor);
    cab1506.setFunction(35,15,60,22,60,10,1,"Headlight",ButtonType.NORMAL,CabFunction.F_LIGHT);
    cab1506.setFunction(35,45,60,22,60,10,0,"Tailight",ButtonType.NORMAL,CabFunction.R_LIGHT);
    cab1506.setFunction(35,75,60,22,60,10,3,"D-Lights",ButtonType.NORMAL,CabFunction.D_LIGHT);


// CREATE MAIN LAYOUT AND DEFINE ALL TRACKS
    
    cab622.setSidingDefaults(rButton6,4,10);      // must set default sidings AFTER rButtons are defined above
    cab6021.setSidingDefaults(rButton1,11,7);
    cab54.setSidingDefaults(rButton2,11,8);
    cab1506.setSidingDefaults(rButton3,11,9);
    cab8601.setSidingDefaults(rButton4,11,12);
    cab1202.setSidingDefaults(rButton5,11,13);
    cab2004.setSidingDefaults(rButton7,5,14);
    
  } // Initialize

//////////////////////////////////////////////////////////////////////////