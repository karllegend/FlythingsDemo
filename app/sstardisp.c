#include "string.h"
#include "stdio.h"
#include "stdlib.h"

#include "mi_sys.h"
#include "sstardisp.h"

#include "mi_panel_datatype.h"
#include "mi_panel.h"
#include "mi_disp_datatype.h"
#include "mi_disp.h"
#include "SAT070CP50_1024x600.h"
//#include "SAT070AT50_800x480.h"

#define UI_1024_600

#define VIDEO_DISP_X 80
#define VIDEO_DISP_Y 60
#define VIDEO_DISP_W 640
#define VIDEO_DISP_H 360

#define VIDEO_DISP_1024_X 100
#define VIDEO_DISP_1024_Y 60
#define VIDEO_DISP_1024_W 704
#define VIDEO_DISP_1024_H 480

#if defined(__cplusplus)||defined(c_plusplus)
extern "C"{
#endif
int sstar_disp_init(MI_DISP_PubAttr_t *pstDispPubAttr)
{
    MI_PANEL_LinkType_e eLinkType;
    MI_DISP_InputPortAttr_t stInputPortAttr;

    memset(&stInputPortAttr, 0, sizeof(stInputPortAttr));

    MI_SYS_Init();

    if (pstDispPubAttr->eIntfType == E_MI_DISP_INTF_LCD)
    {
        pstDispPubAttr->stSyncInfo.u16Vact = stPanelParam.u16Height;
        pstDispPubAttr->stSyncInfo.u16Vbb = stPanelParam.u16VSyncBackPorch;
        pstDispPubAttr->stSyncInfo.u16Vfb = stPanelParam.u16VTotal - (stPanelParam.u16VSyncWidth +
                                                                      stPanelParam.u16Height + stPanelParam.u16VSyncBackPorch);
        pstDispPubAttr->stSyncInfo.u16Hact = stPanelParam.u16Width;
        pstDispPubAttr->stSyncInfo.u16Hbb = stPanelParam.u16HSyncBackPorch;
        pstDispPubAttr->stSyncInfo.u16Hfb = stPanelParam.u16HTotal - (stPanelParam.u16HSyncWidth +
                                                                      stPanelParam.u16Width + stPanelParam.u16HSyncBackPorch);
        pstDispPubAttr->stSyncInfo.u16Bvact = 0;
        pstDispPubAttr->stSyncInfo.u16Bvbb = 0;
        pstDispPubAttr->stSyncInfo.u16Bvfb = 0;
        pstDispPubAttr->stSyncInfo.u16Hpw = stPanelParam.u16HSyncWidth;
        pstDispPubAttr->stSyncInfo.u16Vpw = stPanelParam.u16VSyncWidth;
        pstDispPubAttr->stSyncInfo.u32FrameRate = stPanelParam.u16DCLK * 1000000 / (stPanelParam.u16HTotal * stPanelParam.u16VTotal);
        pstDispPubAttr->eIntfSync = E_MI_DISP_OUTPUT_USER;
        pstDispPubAttr->eIntfType = E_MI_DISP_INTF_LCD;
        eLinkType = E_MI_PNL_LINK_TTL;
    }
    #ifdef UI_1024_600
        stInputPortAttr.u16SrcWidth = 822;//VIDEO_DISP_1024_W;
        stInputPortAttr.u16SrcHeight = 464;//VIDEO_DISP_1024_H;
        stInputPortAttr.stDispWin.u16X = VIDEO_DISP_1024_X;
        stInputPortAttr.stDispWin.u16Y = VIDEO_DISP_1024_Y;
        stInputPortAttr.stDispWin.u16Width = 822;//VIDEO_DISP_1024_W;
        stInputPortAttr.stDispWin.u16Height = 464;//VIDEO_DISP_1024_H;
    #else
        stInputPortAttr.u16SrcWidth = VIDEO_DISP_1024_W;
        stInputPortAttr.u16SrcHeight = VIDEO_DISP_1024_H;
        stInputPortAttr.stDispWin.u16X = VIDEO_DISP_1024_X;
        stInputPortAttr.stDispWin.u16Y = VIDEO_DISP_1024_Y;
        stInputPortAttr.stDispWin.u16Width = VIDEO_DISP_1024_W;
        stInputPortAttr.stDispWin.u16Height = VIDEO_DISP_1024_H;
    #endif

    MI_DISP_SetPubAttr(0, pstDispPubAttr);
    MI_DISP_Enable(0);
    MI_DISP_BindVideoLayer(0, 0);
    MI_DISP_EnableVideoLayer(0);

    MI_DISP_SetInputPortAttr(0, 0, &stInputPortAttr);
    MI_DISP_EnableInputPort(0, 0);
    MI_DISP_SetInputPortSyncMode(0, 0, E_MI_DISP_SYNC_MODE_FREE_RUN);

    if (pstDispPubAttr->eIntfType == E_MI_DISP_INTF_LCD)
    {
        MI_PANEL_Init(eLinkType);
        MI_PANEL_SetPanelParam(&stPanelParam);
        if(eLinkType == E_MI_PNL_LINK_MIPI_DSI)
        {
            //MI_PANEL_SetMipiDsiConfig(&stMipiDsiConfig);
        }
    }
    return 0;
}
int sstar_disp_Deinit(MI_DISP_PubAttr_t *pstDispPubAttr)
{

    MI_DISP_DisableInputPort(0, 0);
    MI_DISP_DisableVideoLayer(0);
    MI_DISP_UnBindVideoLayer(0, 0);
    MI_DISP_Disable(0);

    switch(pstDispPubAttr->eIntfType) {
        case E_MI_DISP_INTF_HDMI:
            break;

        case E_MI_DISP_INTF_VGA:
            break;

        case E_MI_DISP_INTF_LCD:
        default:
            MI_PANEL_DeInit();

    }

    MI_SYS_Exit();
    printf("sstar_disp_Deinit...\n");

    return 0;
}

#if defined(__cplusplus)||defined(c_plusplus)
}
#endif

