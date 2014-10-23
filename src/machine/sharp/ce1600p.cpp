#include <QPainter>
#include <QResource>

#include "common.h"
#include "ce150.h"
#include "ce1600p.h"
#include "cpu.h"
#include "pcxxxx.h"
#include "paperwidget.h"
#include "Keyb.h"
#include "clink.h"
#include "dialoganalog.h"
#include "buspc1500.h"

#define NO_MOVE	0
#define RI_MOVE	1
#define LE_MOVE 2
#define RI_MOVE_MID	3
#define LE_MOVE_MID 4
#define UP_MOVE 1
#define DN_MOVE 2
#define UP_MOVE_MID 3
#define DN_MOVE_MID 4

#define PORT_MOTOR_X (pTIMER->pPC->pCPU->imem[0x83] & 0x0f)
#define PORT_MOTOR_Y ((pTIMER->pPC->pCPU->imem[0x83] >> 4) & 0x0f)
#define PORT_MOTOR_Z (pTIMER->pPC->pCPU->imem[0x82] & 0x0f)

//#define MACRO_ADD_LOG	AddLog(LOG_PRINTER,tr("X=%1 Y=%2 Rot=%3 Color=%4,   IF=%5").arg(Pen_X,Pen_Y,Rot,Pen_Color,pLH5810->lh5810.r_if))
#define MACRO_ADD_LOG	if (pTIMER->pPC->fp_log) fprintf(pTIMER->pPC->fp_log,"X=%d Y=%d Z=%d Rot=%d Color=%d\n",Pen_X,Pen_Y,Pen_Z,Rot,Pen_Color)

Cce1600p::Cce1600p(CPObject *parent) : Cce150(this)
{
    //[constructor]
    BackGroundFname	= P_RES(":/ext/ce-1600p.png");

    setcfgfname(QString("ce1600p"));

    setDXmm(317);//Pc_DX_mm = 317;
    setDYmm(222);//Pc_DY_mm = 222;
    setDZmm(48);//Pc_DZ_mm = 48;

    setDX(1100);//Pc_DX	= 1100;
    setDY(780);//Pc_DY	= 773;

    KeyMap		= KeyMapce1600p;
    KeyMapLenght= KeyMapce1600pLenght;
    delete pKEYB;
    pKEYB		= new Ckeyb(this,"ce1600p.map",0);

    bus = new CbusPc1500();

    memsize			= 0x8000;

    SlotList.clear();
    SlotList.append(CSlot(16, 0x0000 ,P_RES(":/pc1600/romce1600-1.bin"), "" , CSlot::ROM , "ROM"));
    SlotList.append(CSlot(16, 0x4000 ,P_RES(":/pc1600/romce1600-2.bin"), "" , CSlot::ROM , "ROM"));

}

bool Cce1600p::init(void)
{

        CPObject::init();

    #if 0
        QResource res(P_RES(":/ext/clac2.wav"));
        clac = FSOUND_Sample_Load(FSOUND_FREE, (const char*) res.data(), FSOUND_LOADMEMORY, 0, res.size());
    #endif

        setfrequency( 0);

        pCONNECTOR	= new Cconnector(this,60,0,Cconnector::Sharp_60,"Connector 60 pins",true,QPoint(424,536));	publish(pCONNECTOR);


        WatchPoint.add(&pCONNECTOR_value,64,60,this,"Standard 60pins connector");

        AddLog(LOG_PRINTER,tr("PRT initializing..."));

        if(pKEYB)	pKEYB->init();
        if(pTIMER)	pTIMER->init();

        // Create CE-150 Paper Image
        ce150buf	= new QImage(QSize(1920, 3000),QImage::Format_ARGB32);
        ce150display= new QImage(QSize(1920, 567),QImage::Format_ARGB32);
        ce150pen	= new QImage(P_RES(":/ext/ce-150pen.png"));
        // Fill it blank
        clearPaper();

        // Create a paper widget
        paperWidget = new CpaperWidget(QRect(195,30,695,170),ce150buf,this);
        paperWidget->show();

        initsound();


    return true;
}

/* disk
& 78 bit 3 0: drive is empty
1: drive contains disk
Bit 6 0: The drive is or the write protection is active
1: drive is running and write protection is inactive
Bit 7 0: Drive is running
1: drive is
& 79 Bit 0 The three bits together form the.
Bit 1 number of the currently selected bit 2 Track
7A & Bit 7 Write:
0: off drive motor
1: Drive motor on

*/


bool Cce1600p::run(void)
{
    bool has_moved_X = false;
    bool has_moved_Y = false;
    bool has_moved_Z = false;
    bool forwardBus = true;
    bool keyEvent = false;

    bus->fromUInt64(pCONNECTOR->Get_values());

//	lh5810_write();

    ////////////////////////////////////////////////////////////////////
    //	VOLTAGE OK :-)
    //////////////////////////////////////////////////////////////////
//	pLH5810->SetRegBit(LH5810_OPB,6,FALSE);

    ////////////////////////////////////////////////////////////////////
    //	PRINT MODE
    //////////////////////////////////////////////////////////////////
    if (pKEYB->LastKey==K_PRINT_ON)
    {
        Print_Mode = true;
        pKEYB->LastKey = 0;
    }
    if (pKEYB->LastKey==K_PRINT_OFF)
    {
        Print_Mode = false;
        pKEYB->LastKey = 0;
    }
    if (pTIMER->pPC) PUT_BIT(pTIMER->pPC->pCPU->imem[0x81],4,Print_Mode);

    ////////////////////////////////////////////////////////////////////
    //	PAPER FEED
    //////////////////////////////////////////////////////////////////
    if (pTIMER->pPC) PUT_BIT(pTIMER->pPC->pCPU->imem[0x81],1,(pKEYB->LastKey==K_PFEED));

    ////////////////////////////////////////////////////////////////////
    //	REVERSE PAPER FEED
    //////////////////////////////////////////////////////////////////
    if (pTIMER->pPC) PUT_BIT(pTIMER->pPC->pCPU->imem[0x81],2,(pKEYB->LastKey==K_PBFEED));


    ////////////////////////////////////////////////////////////////////
    //	RMT ON/OFF
    ////////////////////////////////////////////////////////////////////
//	if (pLH5810->lh5810.r_opa & 0x02)	((Cpc15XX *)pPC->pTIMER->pPC)->pce152->paused = FALSE;	// RMT 0 ON
//	if (pLH5810->lh5810.r_opa & 0x04)	((Cpc15XX *)pPC->pTIMER->pPC)->pce152->paused = TRUE;	// RMT 0 OFF

#if 0
    if (!bus->isEnable()) {
        if (keyEvent) {
            pLH5810->step();
            bus->setINT(pLH5810->INT);
            pCONNECTOR->Set_values(bus->toUInt64());
        }
        return true;
    }

    quint16 addr = bus->getAddr();

    if (bus->isEnable() &&
        !bus->isME1() &&
        !bus->isPV() &&
        !bus->isWrite() &&
        (addr >=0xA000) && (addr < 0xC000) )
    {
        bus->setData(mem[addr - 0xA000]);
        forwardBus = false;
        bus->setEnable(false);
        pCONNECTOR->Set_values(bus->toUInt64());
        return true;
    }

    if (bus->isEnable() &&
        bus->isME1() &&
        (addr >= 0xb000) &&
        (addr <= 0xb00f) &&
        (bus->isWrite()) )
    {
        lh5810_write();
        forwardBus = false;
    }
#endif

    Direction = Motor_X.SendPhase(PORT_MOTOR_X);

    switch (Direction)
    {
        case RI_MOVE:   Pen_X++;
        case RI_MOVE_MID:	Pen_X++;
                        has_moved_X=true;
                        MACRO_ADD_LOG;
                        break;
        case LE_MOVE:   Pen_X--;
        case LE_MOVE_MID:   Pen_X--;
                        has_moved_X=true;
                        MACRO_ADD_LOG;
                        break;
    }

    Direction = Motor_Y.SendPhase(PORT_MOTOR_Y);

    switch (Direction)
    {
    case UP_MOVE:       Pen_Y--;
    case UP_MOVE_MID:   Pen_Y--;
                        has_moved_Y=true;
                        MACRO_ADD_LOG;
                        break;
    case DN_MOVE:       Pen_Y++;
    case DN_MOVE_MID:	Pen_Y++;
                        has_moved_Y=true;
                        MACRO_ADD_LOG;
                        break;
    }

    Direction = Motor_Z.SendPhase(PORT_MOTOR_Z);

    switch (Direction)
    {
    case UP_MOVE:       Pen_Z--;
    case UP_MOVE_MID:   Pen_Z--;
                        has_moved_Z=true;
                        MACRO_ADD_LOG;
                        break;
    case DN_MOVE:       Pen_Z++;
    case DN_MOVE_MID:	Pen_Z++;
                        has_moved_Z=true;
                        MACRO_ADD_LOG;
                        break;
    }

    if (has_moved_Z)
    {
        if (Pen_Z < 0) Pen_Z = 0;
        if (Pen_Z >50) Pen_Z = 50;
        if (Pen_Z == 6) // Pen up
        {
#ifndef NO_SOUND
//            clac->play();
            if (getfrequency()>0) {
                fillSoundBuffer(0xFF);
                fillSoundBuffer(0x00);
            }
            else {
//                int ps = m_audioOutput->periodSize();
                mainwindow->audioMutex.lock();
                QByteArray buff;
                buff.append(0xFF);
                buff.append((char)0);
                m_output->write(buff.constData(),2);
                mainwindow->audioMutex.unlock();
            }
#endif

            Pen_Status = PEN_UP;
            AddLog(LOG_PRINTER,"PEN UP");
        }
        if (Pen_Z == 0) {   // Pen down
#if 0
            int iChanIndex = FSOUND_PlaySoundEx(FSOUND_FREE, clac,0,true);
            FSOUND_SetVolumeAbsolute(iChanIndex,255);
            FSOUND_SetPaused(iChanIndex,false);
#endif
#ifndef NO_SOUND
//            clac->play();
            if (getfrequency()>0) {
                fillSoundBuffer(0xFF);
                fillSoundBuffer(0x00);
            }
            else {
//                int ps = m_audioOutput->periodSize();
                mainwindow->audioMutex.lock();
                QByteArray buff;
                buff.append(0xFF);
                buff.append((char)0);
                m_output->write(buff.constData(),2);
                mainwindow->audioMutex.unlock();
            }
#endif
            Pen_Status = PEN_DOWN;
            AddLog(LOG_PRINTER,"PEN DOWN");
        }

        if (Pen_Z == 20) { // change color position
            StartRot = true;	// rotation pin engaged
        }
        if ((Pen_Z == 50) && StartRot) {
            Rot++;
            StartRot = false;
            // If the third 1/12 rotation, then put next color
            if (Rot == 8) {
                Rot = 0;
                Next_Color();
            }
        }

    }


//    pTIMER->pPC->pCPU->imem[0x81] |= (Pen_X <= 0 ? 0x20 : 0x00);
    // Left position detection
    if (pTIMER->pPC) PUT_BIT(pTIMER->pPC->pCPU->imem[0x81],5,Pen_X <= 0);

    //---------------------------------------------------
    // Draw printer
    //---------------------------------------------------
    if (has_moved_Y || (has_moved_X && (Pen_Status==PEN_DOWN))) Print();


    pCONNECTOR->Set_pin(1	,1);
//    pCONNECTOR->Set_pin(30	,pLH5810->INT);

    return(1);
}


void Cce1600p::Print(void)
{
    QPainter painter;

    pPC->Refresh_Display = true;

    if (Pen_Status==PEN_DOWN)
    {
        painter.begin(ce150buf);
        switch (Pen_Color)
        {
            case 0 : painter.setPen( Qt::black); break;
            case 1 : painter.setPen( Qt::blue); break;
            case 2 : painter.setPen( Qt::green); break;
            case 3 : painter.setPen( Qt::red); break;
        }
        painter.drawPoint( Pen_X, Pen_Y );
        painter.end();
    }

    paperWidget->setOffset(QPoint(0,Pen_Y));
}
