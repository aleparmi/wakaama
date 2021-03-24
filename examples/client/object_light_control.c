/*******************************************************************************
 *
 * Copyright (c) 2013, 2014 Intel Corporation and others.
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v2.0
 * and Eclipse Distribution License v1.0 which accompany this distribution.
 *
 * The Eclipse Public License is available at
 *    http://www.eclipse.org/legal/epl-v20.html
 * The Eclipse Distribution License is available at
 *    http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    David Navarro, Intel Corporation - initial API and implementation
 *    domedambrosio - Please refer to git log
 *    Fabien Fleutot - Please refer to git log
 *    Axel Lorente - Please refer to git log
 *    Bosch Software Innovations GmbH - Please refer to git log
 *    Pascal Rieux - Please refer to git log
 *    Scott Bertin, AMETEK, Inc. - Please refer to git log
 *    
 *******************************************************************************/

/*
 Copyright (c) 2013, 2014 Intel Corporation

 Redistribution and use in source and binary forms, with or without modification,
 are permitted provided that the following conditions are met:

     * Redistributions of source code must retain the above copyright notice,
       this list of conditions and the following disclaimer.
     * Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.
     * Neither the name of Intel Corporation nor the names of its contributors
       may be used to endorse or promote products derived from this software
       without specific prior written permission.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
 INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 THE POSSIBILITY OF SUCH DAMAGE.

 David Navarro <david.navarro@intel.com>

*/

/*
 * This object is single instance only, and is mandatory to all LWM2M light control as it describe the object such as its
 * manufacturer, model, etc...
 */

#include "liblwm2m.h"
#include "lwm2mclient.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

//AT Commands libraries and constant definitions for BG96
#include <wiringPi.h>
#include <wiringSerial.h>

#define DEVICE "/dev/ttyUSB2"
#define BAUD 115200

int fd; //global variable for serial communication

#define OFF               false

#define PRV_TLV_BUFFER_SIZE 128

// Resource Id's:
#define RES_M_ONOFF 5850

typedef struct
{
    bool light_status;
} light_control_data_t;


int send_at_light(int status) {
    if (status == 0) {
        serialPuts(fd, "AT+QCFG=\"ledmode\", 0\r");
        printf("AT+QCFG=\"ledmode\", 0\n");

        delay(500) ;

        while(serialDataAvail(fd)) {
            printf("%c", serialGetchar (fd));
            fflush (stdout);
        }
        printf("\n");
    }
    else {
        serialPuts(fd, "AT+QCFG=\"ledmode\", 1\r");
        printf("AT+QCFG=\"ledmode\", 1\n");

        delay(500) ;

        while(serialDataAvail(fd)) {
            printf("%c", serialGetchar (fd));
            fflush (stdout);
        }
        printf("\n");
    }
    return 0;
}

static uint8_t prv_set_value(lwm2m_data_t * dataP,
                             light_control_data_t * devDataP)
{
    lwm2m_data_t * subTlvP;
    size_t count;
    size_t i;
    // a simple switch structure is used to respond at the specified resource asked
    switch (dataP->id)
    {
    case RES_M_ONOFF:
        if (dataP->type == LWM2M_TYPE_MULTIPLE_RESOURCE) return COAP_404_NOT_FOUND;
        lwm2m_data_encode_bool(devDataP->light_status, dataP);
        return COAP_205_CONTENT;
    default:
        return COAP_404_NOT_FOUND;
    }
}

static uint8_t prv_light_control_read(uint16_t instanceId,
                               int * numDataP,
                               lwm2m_data_t ** dataArrayP,
                               lwm2m_object_t * objectP)
{
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
                RES_M_ONOFF
        };
        int nbRes = sizeof(resList)/sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0 ; i < nbRes ; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }

    i = 0;
    do
    {
        result = prv_set_value((*dataArrayP) + i, (light_control_data_t*)(objectP->userData));
        i++;
    } while (i < *numDataP && result == COAP_205_CONTENT);

    return result;
}

static uint8_t prv_light_control_discover(uint16_t instanceId,
                                   int * numDataP,
                                   lwm2m_data_t ** dataArrayP,
                                   lwm2m_object_t * objectP)
{
    uint8_t result;
    int i;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    result = COAP_205_CONTENT;

    // is the server asking for the full object ?
    if (*numDataP == 0)
    {
        uint16_t resList[] = {
                RES_M_ONOFF
        };
        int nbRes = sizeof(resList) / sizeof(uint16_t);

        *dataArrayP = lwm2m_data_new(nbRes);
        if (*dataArrayP == NULL) return COAP_500_INTERNAL_SERVER_ERROR;
        *numDataP = nbRes;
        for (i = 0; i < nbRes; i++)
        {
            (*dataArrayP)[i].id = resList[i];
        }
    }
    else
    {
        for (i = 0; i < *numDataP && result == COAP_205_CONTENT; i++)
        {
            switch ((*dataArrayP)[i].id)
            {
            case RES_M_ONOFF:
                break;
            default:
                result = COAP_404_NOT_FOUND;
            }
        }
    }

    return result;
}

static uint8_t prv_light_control_write(uint16_t instanceId,
                                int numData,
                                lwm2m_data_t * dataArray,
                                lwm2m_object_t * objectP,
                                lwm2m_write_type_t writeType)
{
    int i;
    uint8_t result;
    bool value;

    // All write types are treated the same here
    (void)writeType;

    // this is a single instance object
    if (instanceId != 0)
    {
        return COAP_404_NOT_FOUND;
    }

    light_control_data_t* light_control = (light_control_data_t *)
            lwm2m_list_find(objectP->instanceList, instanceId);

    i = 0;

    do
    {
        /* No multiple instance resources */
        if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
            result = COAP_404_NOT_FOUND;
            continue;
        }

        switch (dataArray[i].id)
        {
        case RES_M_ONOFF:

            if (1 == lwm2m_data_decode_bool(dataArray + i, &value))
            {
                light_control->light_status = value;
                if(send_at_light(light_control->light_status) == 0) {
                    result = COAP_204_CHANGED;
                }
            }
            else
            {
                result = COAP_400_BAD_REQUEST;
            }
            break;
        default:
            result = COAP_405_METHOD_NOT_ALLOWED;
        }

        i++;
    } while (i < numData && result == COAP_204_CHANGED);

    return result;
}

void display_light_control_object(lwm2m_object_t * object)
{
    light_control_data_t * data = (light_control_data_t *)object->userData;
    fprintf(stdout, "  /%u: Light Control Object:\r\n", object->objID);
    if (NULL != data)
    {
        fprintf(stdout, "    light status: %d\r\n", data->light_status);
    }
}

lwm2m_object_t * get_light_control_object()
{
    /*
     * The get_object_light_control function create the object itself and return a pointer to the structure that represent it.
     */
    lwm2m_object_t * lightControlObj;

    lightControlObj = (lwm2m_object_t *)lwm2m_malloc(sizeof(lwm2m_object_t));

    if (NULL != lightControlObj)
    {
        memset(lightControlObj, 0, sizeof(lwm2m_object_t));

        /*
         * It assigns his unique ID
         * The 3 is the standard ID for the mandatory object "Light Control Object".
         */
        lightControlObj->objID = LIGHT_CONTROL_OBJECT_ID;

        /*
         * and its unique instance
         *
         */
        lightControlObj->instanceList = (lwm2m_list_t *)lwm2m_malloc(sizeof(lwm2m_list_t));
        if (NULL != lightControlObj->instanceList)
        {
            memset(lightControlObj->instanceList, 0, sizeof(lwm2m_list_t));
        }
        else
        {
            lwm2m_free(lightControlObj);
            return NULL;
        }
        
        /*
         * And the private function that will access the object.
         * Those function will be called when a read/write/execute query is made by the server. In fact the library don't need to
         * know the resources of the object, only the server does.
         */
        lightControlObj->readFunc     = prv_light_control_read;
        lightControlObj->discoverFunc = prv_light_control_discover;
        lightControlObj->writeFunc    = prv_light_control_write;
        lightControlObj->userData = lwm2m_malloc(sizeof(light_control_data_t));

        /*
         * Also some user data can be stored in the object with a private structure containing the needed variables 
         */
    
        fd = serialOpen(DEVICE, BAUD);

        if (fd >= 0) {
            fprintf(stdout, "\nSerial Port opened!\n\n");

            serialPuts(fd, "ATE0\r");
            fprintf(stdout, "ATE0\n");

            delay(500) ;

            while(serialDataAvail(fd)) {
                fprintf(stdout, "%c", serialGetchar (fd));
                fflush (stdout);
            }
            fprintf(stdout, "\n");
        }
        else {
            fprintf(stderr, "Error in opening the COM port for the light object!\n");
            lwm2m_free(lightControlObj->instanceList);
            lwm2m_free(lightControlObj);
            lightControlObj = NULL;
        }
        if (NULL != lightControlObj->userData)
        {
            int light_blink = 0;
            if (send_at_light(light_blink) == 0) {
                ((light_control_data_t*)lightControlObj->userData)->light_status = OFF;
            }
        }
        else
        {
            lwm2m_free(lightControlObj->instanceList);
            lwm2m_free(lightControlObj);
            lightControlObj = NULL;
        }
    }

    return lightControlObj;
}

void free_object_light_control(lwm2m_object_t * objectP)
{
    if (NULL != objectP->userData)
    {
        lwm2m_free(objectP->userData);
        objectP->userData = NULL;
    }
    if (NULL != objectP->instanceList)
    {
        lwm2m_free(objectP->instanceList);
        objectP->instanceList = NULL;
    }

    lwm2m_free(objectP);
}
