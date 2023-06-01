#include "DSP_4W.h"
#include "util.h"

void DSP_4W::setup(int dsp_tx, int dsp_rx, int dummy, int dummy2)
{
    _dsp_serial.begin(9600, SWSERIAL_8N1, dsp_tx, dsp_rx, false, 63);
    _dsp_serial.setTimeout(20);
    dsp_toggles.locked_change = 0;
    dsp_toggles.power_change = 0;
    dsp_toggles.unit_change = 0;
    // dsp_toggles.error = 0;
    dsp_toggles.pressed_button = NOBTN;
    dsp_toggles.no_of_heater_elements_on = 2;
    dsp_toggles.godmode = 0;
}

void DSP_4W::stop()
{
    _dsp_serial.stopListening();
}

void DSP_4W::pause_resume(bool action)
{
    if(action)
    {
        _dsp_serial.stopListening();
    } else
    {
        _dsp_serial.listen();
    }
}

void DSP_4W::updateToggles()
{
    /*We don't need a message from dsp to update these. Moved here from line81*/
    dsp_toggles.godmode = dsp_states.godmode;
    dsp_toggles.target = dsp_states.target;

    int msglen = 0;
    //check if display sent a message
    msglen = 0;
    if(!_dsp_serial.available()) return;
    uint8_t tempbuffer[PAYLOADSIZE];
    msglen = _dsp_serial.readBytes(tempbuffer, PAYLOADSIZE);
    if(msglen != PAYLOADSIZE) return;

    //discard message if checksum is wrong
    uint8_t calculatedChecksum;
    calculatedChecksum = tempbuffer[1]+tempbuffer[2]+tempbuffer[3]+tempbuffer[4];
    if(tempbuffer[DSP_CHECKSUMINDEX] != calculatedChecksum)
    {
        return;
    }
    /*message is good if we get here. Continue*/

    good_packets_count++;
    /* Copy tempbuffer into _from_DSP_buf */
    for(int i = 0; i < PAYLOADSIZE; i++)
        {
            _from_DSP_buf[i] = tempbuffer[i];
            _raw_payload_from_dsp[i] = tempbuffer[i];
        }

    uint8_t bubbles = (_from_DSP_buf[COMMANDINDEX] & getBubblesBitmask()) > 0;
    uint8_t pump = (_from_DSP_buf[COMMANDINDEX] & getPumpBitmask()) > 0;
    uint8_t jets = (_from_DSP_buf[COMMANDINDEX] & getJetsBitmask()) > 0;

    if(dsp_states.godmode)
    {
        /*0 = no change, 1 = toggle for these fields*/
        dsp_toggles.bubbles_change = _bubbles != bubbles;
        dsp_toggles.heat_change = 0;
        dsp_toggles.jets_change = _jets != jets;
        dsp_toggles.locked_change = 0;
        dsp_toggles.power_change = 0;
        dsp_toggles.pump_change = _pump != pump;
        dsp_toggles.unit_change = 0;
        /*Absolute values*/
        dsp_toggles.no_of_heater_elements_on = 2;
        dsp_toggles.pressed_button = NOBTN;
        // dsp_toggles.godmode = 1;
        // dsp_toggles.target = dsp_states.target;
    }
    
    /*MAYBE toggle states if godmode and user pressed buttons on display which leads to changes in states*/
    _bubbles = (_from_DSP_buf[COMMANDINDEX] & getBubblesBitmask()) > 0;
    // dsp_toggles.heatgrn = 2;                           //unknowable in antigodmode
    // dsp_toggles.heatred = (_from_DSP_buf[COMMANDINDEX] & (getHeatBitmask1() | getHeatBitmask2())) > 0;
    _pump = (_from_DSP_buf[COMMANDINDEX] & getPumpBitmask()) > 0;
    _jets = (_from_DSP_buf[COMMANDINDEX] & getJetsBitmask()) > 0;
    // dsp_toggles.no_of_heater_elements_on = (_from_DSP_buf[COMMANDINDEX] & getHeatBitmask2()) > 0;

    /*This is placed here to send messages at the same rate as the dsp.*/
    _dsp_serial.write(_to_DSP_buf, PAYLOADSIZE);
    return;
}

void DSP_4W::handleStates()
{
    /* If godmode - generate payload, else send rawpayload*/
    if(dsp_states.godmode)
    {
        generatePayload();
    }
    else
    {
        if(PAYLOADSIZE > _raw_payload_to_dsp.size()) 
        {
            return;
        }
        for(int i = 0; i < PAYLOADSIZE; i++)
        {
            _to_DSP_buf[i] = _raw_payload_to_dsp[i];
        }
    }
}

void DSP_4W::generatePayload()
{
    int tempC;
    dsp_states.unit ? tempC = dsp_states.temperature : tempC = F2C(dsp_states.temperature);

    _to_DSP_buf[0] = B10101010;
    _to_DSP_buf[1] = 2;
    _to_DSP_buf[2] = tempC;
    _to_DSP_buf[3] = dsp_states.error;
    _to_DSP_buf[4] = 0;
    _to_DSP_buf[5] = _to_DSP_buf[1]+_to_DSP_buf[2]+_to_DSP_buf[3]+_to_DSP_buf[4];
    _to_DSP_buf[6] = B10101010;
}