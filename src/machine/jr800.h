#ifndef JR800_H
#define JR800_H

class CPObject;
class Cupd7907;
class CUPD16434;
class CPD1990AC;
class Cconnector;

#include "pcxxxx.h"


class Cjr800 : public CpcXXXX
{
    Q_OBJECT

public:
    Cjr800(CPObject *parent = 0);
    virtual ~Cjr800();

    virtual bool	Chk_Adr(UINT32 *d,UINT32 data);
    virtual bool	Chk_Adr_R(UINT32 *d, UINT32 *data);
    virtual UINT8 in(UINT8 address);
    virtual UINT8 out(UINT8 address,UINT8 value);
    virtual UINT16 out16(UINT16 address,UINT16 value);

    virtual bool	Set_Connector(Cbus *_bus = 0);
    virtual bool	Get_Connector(Cbus *_bus = 0);



    virtual bool init();
    virtual bool run();
    virtual void Reset();

    void TurnON();
    void TurnOFF();
    bool SaveConfig(QXmlStreamWriter *xmlOut);
    bool LoadConfig(QXmlStreamReader *xmlIn);

    UINT16 getKey();

    CUPD16434 *upd16434[4];

    Cconnector	*pTAPECONNECTOR;	qint64 pTAPECONNECTOR_value;
    Cconnector	*pPRINTERCONNECTOR;	qint64 pPRINTERCONNECTOR_value;
private:
    UINT16 kstrobe;
};

#endif // JR800_H

