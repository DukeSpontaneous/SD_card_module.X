#
# Generated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Include project Makefile
ifeq "${IGNORE_LOCAL}" "TRUE"
# do not include local makefile. User is passing all local related variables already
else
include Makefile
# Include makefile containing local settings
ifeq "$(wildcard nbproject/Makefile-local-pic24hj128gp502_sd_module.mk)" "nbproject/Makefile-local-pic24hj128gp502_sd_module.mk"
include nbproject/Makefile-local-pic24hj128gp502_sd_module.mk
endif
endif

# Environment
MKDIR=gnumkdir -p
RM=rm -f 
MV=mv 
CP=cp 

# Macros
CND_CONF=pic24hj128gp502_sd_module
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
IMAGE_TYPE=debug
OUTPUT_SUFFIX=elf
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
else
IMAGE_TYPE=production
OUTPUT_SUFFIX=hex
DEBUGGABLE_SUFFIX=elf
FINAL_IMAGE=dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}
endif

ifeq ($(COMPARE_BUILD), true)
COMPARISON_BUILD=-mafrlcsj
else
COMPARISON_BUILD=
endif

ifdef SUB_IMAGE_ADDRESS
SUB_IMAGE_ADDRESS_COMMAND=--image-address $(SUB_IMAGE_ADDRESS)
else
SUB_IMAGE_ADDRESS_COMMAND=
endif

# Object Directory
OBJECTDIR=build/${CND_CONF}/${IMAGE_TYPE}

# Distribution Directory
DISTDIR=dist/${CND_CONF}/${IMAGE_TYPE}

# Source Files Quoted if spaced
SOURCEFILES_QUOTED_IF_SPACED=app/system_config/pic24hj128gp502_sd_module/system.c app/system_config/pic24hj128gp502_sd_module/system_time.c app/main.c framework/driver/spi/src/drv_spi_16bit.c framework/fileio/src/fileio.c framework/fileio/drivers/sd_spi/sd_spi.c mydrivers/fileio/ringstore_fat32.c mydrivers/rs-485/rs-485.c mydrivers/rs-485/rs_legasy.c mydrivers/timers/timers.c myprotocols/shuttle_control_system/slave_modul_sd.c

# Object Files Quoted if spaced
OBJECTFILES_QUOTED_IF_SPACED=${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o ${OBJECTDIR}/app/main.o ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o ${OBJECTDIR}/framework/fileio/src/fileio.o ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o ${OBJECTDIR}/mydrivers/rs-485/rs-485.o ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o ${OBJECTDIR}/mydrivers/timers/timers.o ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o
POSSIBLE_DEPFILES=${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o.d ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o.d ${OBJECTDIR}/app/main.o.d ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o.d ${OBJECTDIR}/framework/fileio/src/fileio.o.d ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o.d ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o.d ${OBJECTDIR}/mydrivers/rs-485/rs-485.o.d ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o.d ${OBJECTDIR}/mydrivers/timers/timers.o.d ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o.d

# Object Files
OBJECTFILES=${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o ${OBJECTDIR}/app/main.o ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o ${OBJECTDIR}/framework/fileio/src/fileio.o ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o ${OBJECTDIR}/mydrivers/rs-485/rs-485.o ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o ${OBJECTDIR}/mydrivers/timers/timers.o ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o

# Source Files
SOURCEFILES=app/system_config/pic24hj128gp502_sd_module/system.c app/system_config/pic24hj128gp502_sd_module/system_time.c app/main.c framework/driver/spi/src/drv_spi_16bit.c framework/fileio/src/fileio.c framework/fileio/drivers/sd_spi/sd_spi.c mydrivers/fileio/ringstore_fat32.c mydrivers/rs-485/rs-485.c mydrivers/rs-485/rs_legasy.c mydrivers/timers/timers.c myprotocols/shuttle_control_system/slave_modul_sd.c


CFLAGS=
ASFLAGS=
LDLIBSOPTIONS=

############# Tool locations ##########################################
# If you copy a project from one host to another, the path where the  #
# compiler is installed may be different.                             #
# If you open this project with MPLAB X in the new host, this         #
# makefile will be regenerated and the paths will be corrected.       #
#######################################################################
# fixDeps replaces a bunch of sed/cat/printf statements that slow down the build
FIXDEPS=fixDeps

.build-conf:  ${BUILD_SUBPROJECTS}
ifneq ($(INFORMATION_MESSAGE), )
	@echo $(INFORMATION_MESSAGE)
endif
	${MAKE}  -f nbproject/Makefile-pic24hj128gp502_sd_module.mk dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}

MP_PROCESSOR_OPTION=24HJ128GP502
MP_LINKER_FILE_OPTION=,--script=p24HJ128GP502.gld
# ------------------------------------------------------------------------------------
# Rules for buildStep: compile
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o: app/system_config/pic24hj128gp502_sd_module/system.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module" 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o.d 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  app/system_config/pic24hj128gp502_sd_module/system.c  -o ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o: app/system_config/pic24hj128gp502_sd_module/system_time.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module" 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o.d 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  app/system_config/pic24hj128gp502_sd_module/system_time.c  -o ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/app/main.o: app/main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/app" 
	@${RM} ${OBJECTDIR}/app/main.o.d 
	@${RM} ${OBJECTDIR}/app/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  app/main.c  -o ${OBJECTDIR}/app/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/app/main.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/app/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o: framework/driver/spi/src/drv_spi_16bit.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/framework/driver/spi/src" 
	@${RM} ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o.d 
	@${RM} ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  framework/driver/spi/src/drv_spi_16bit.c  -o ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/framework/fileio/src/fileio.o: framework/fileio/src/fileio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/framework/fileio/src" 
	@${RM} ${OBJECTDIR}/framework/fileio/src/fileio.o.d 
	@${RM} ${OBJECTDIR}/framework/fileio/src/fileio.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  framework/fileio/src/fileio.c  -o ${OBJECTDIR}/framework/fileio/src/fileio.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/framework/fileio/src/fileio.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/framework/fileio/src/fileio.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o: framework/fileio/drivers/sd_spi/sd_spi.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/framework/fileio/drivers/sd_spi" 
	@${RM} ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o.d 
	@${RM} ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  framework/fileio/drivers/sd_spi/sd_spi.c  -o ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o: mydrivers/fileio/ringstore_fat32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/fileio" 
	@${RM} ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/fileio/ringstore_fat32.c  -o ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/rs-485/rs-485.o: mydrivers/rs-485/rs-485.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/rs-485" 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs-485.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs-485.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/rs-485/rs-485.c  -o ${OBJECTDIR}/mydrivers/rs-485/rs-485.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/rs-485/rs-485.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/rs-485/rs-485.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o: mydrivers/rs-485/rs_legasy.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/rs-485" 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/rs-485/rs_legasy.c  -o ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/timers/timers.o: mydrivers/timers/timers.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/timers" 
	@${RM} ${OBJECTDIR}/mydrivers/timers/timers.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/timers/timers.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/timers/timers.c  -o ${OBJECTDIR}/mydrivers/timers/timers.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/timers/timers.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/timers/timers.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o: myprotocols/shuttle_control_system/slave_modul_sd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/myprotocols/shuttle_control_system" 
	@${RM} ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o.d 
	@${RM} ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  myprotocols/shuttle_control_system/slave_modul_sd.c  -o ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o.d"      -g -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1    -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
else
${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o: app/system_config/pic24hj128gp502_sd_module/system.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module" 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o.d 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  app/system_config/pic24hj128gp502_sd_module/system.c  -o ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o: app/system_config/pic24hj128gp502_sd_module/system_time.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module" 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o.d 
	@${RM} ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  app/system_config/pic24hj128gp502_sd_module/system_time.c  -o ${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/app/system_config/pic24hj128gp502_sd_module/system_time.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/app/main.o: app/main.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/app" 
	@${RM} ${OBJECTDIR}/app/main.o.d 
	@${RM} ${OBJECTDIR}/app/main.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  app/main.c  -o ${OBJECTDIR}/app/main.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/app/main.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/app/main.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o: framework/driver/spi/src/drv_spi_16bit.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/framework/driver/spi/src" 
	@${RM} ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o.d 
	@${RM} ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  framework/driver/spi/src/drv_spi_16bit.c  -o ${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/framework/driver/spi/src/drv_spi_16bit.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/framework/fileio/src/fileio.o: framework/fileio/src/fileio.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/framework/fileio/src" 
	@${RM} ${OBJECTDIR}/framework/fileio/src/fileio.o.d 
	@${RM} ${OBJECTDIR}/framework/fileio/src/fileio.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  framework/fileio/src/fileio.c  -o ${OBJECTDIR}/framework/fileio/src/fileio.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/framework/fileio/src/fileio.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/framework/fileio/src/fileio.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o: framework/fileio/drivers/sd_spi/sd_spi.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/framework/fileio/drivers/sd_spi" 
	@${RM} ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o.d 
	@${RM} ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  framework/fileio/drivers/sd_spi/sd_spi.c  -o ${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/framework/fileio/drivers/sd_spi/sd_spi.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o: mydrivers/fileio/ringstore_fat32.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/fileio" 
	@${RM} ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/fileio/ringstore_fat32.c  -o ${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/fileio/ringstore_fat32.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/rs-485/rs-485.o: mydrivers/rs-485/rs-485.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/rs-485" 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs-485.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs-485.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/rs-485/rs-485.c  -o ${OBJECTDIR}/mydrivers/rs-485/rs-485.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/rs-485/rs-485.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/rs-485/rs-485.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o: mydrivers/rs-485/rs_legasy.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/rs-485" 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/rs-485/rs_legasy.c  -o ${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/rs-485/rs_legasy.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/mydrivers/timers/timers.o: mydrivers/timers/timers.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/mydrivers/timers" 
	@${RM} ${OBJECTDIR}/mydrivers/timers/timers.o.d 
	@${RM} ${OBJECTDIR}/mydrivers/timers/timers.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  mydrivers/timers/timers.c  -o ${OBJECTDIR}/mydrivers/timers/timers.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/mydrivers/timers/timers.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/mydrivers/timers/timers.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o: myprotocols/shuttle_control_system/slave_modul_sd.c  nbproject/Makefile-${CND_CONF}.mk
	@${MKDIR} "${OBJECTDIR}/myprotocols/shuttle_control_system" 
	@${RM} ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o.d 
	@${RM} ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o 
	${MP_CC} $(MP_EXTRA_CC_PRE)  myprotocols/shuttle_control_system/slave_modul_sd.c  -o ${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o  -c -mcpu=$(MP_PROCESSOR_OPTION)  -MMD -MF "${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o.d"        -g -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -O0 -I"app" -I"framework" -I"framework/fileio/src" -I"framework/fileio/inc" -I"app/system_config/pic24hj128gp502_sd_module" -I"framework/driver/spi" -I"framework/fileio/drivers/sd_spi" -I"mydrivers/fileio" -I"mydrivers/rs-485" -I"mydrivers/timers" -I"myprotocols/shuttle_control_system" -msmart-io=1 -Wall -msfr-warn=off  
	@${FIXDEPS} "${OBJECTDIR}/myprotocols/shuttle_control_system/slave_modul_sd.o.d" $(SILENT)  -rsi ${MP_CC_DIR}../ 
	
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemble
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: assemblePreproc
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
else
endif

# ------------------------------------------------------------------------------------
# Rules for buildStep: link
ifeq ($(TYPE_IMAGE), DEBUG_RUN)
dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk    
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -D__DEBUG -D__MPLAB_DEBUGGER_ICD3=1  -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)   -mreserve=data@0x800:0x81F -mreserve=data@0x820:0x821 -mreserve=data@0x822:0x823 -mreserve=data@0x824:0x825 -mreserve=data@0x826:0x84F   -Wl,,,--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--defsym=__DEBUG=1,--defsym=__MPLAB_DEBUGGER_ICD3=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	
else
dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${OUTPUT_SUFFIX}: ${OBJECTFILES}  nbproject/Makefile-${CND_CONF}.mk   
	@${MKDIR} dist/${CND_CONF}/${IMAGE_TYPE} 
	${MP_CC} $(MP_EXTRA_LD_PRE)  -o dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX}  ${OBJECTFILES_QUOTED_IF_SPACED}      -mcpu=$(MP_PROCESSOR_OPTION)        -omf=elf -DXPRJ_pic24hj128gp502_sd_module=$(CND_CONF)  -legacy-libc  $(COMPARISON_BUILD)  -Wl,,,--defsym=__MPLAB_BUILD=1,$(MP_LINKER_FILE_OPTION),--stack=16,--check-sections,--data-init,--pack-data,--handles,--isr,--no-gc-sections,--fill-upper=0,--stackguard=16,--no-force-link,--smart-io,-Map="${DISTDIR}/${PROJECTNAME}.${IMAGE_TYPE}.map",--report-mem,--memorysummary,dist/${CND_CONF}/${IMAGE_TYPE}/memoryfile.xml$(MP_EXTRA_LD_POST) 
	${MP_CC_DIR}\\xc16-bin2hex dist/${CND_CONF}/${IMAGE_TYPE}/SD_card_module.X.${IMAGE_TYPE}.${DEBUGGABLE_SUFFIX} -a  -omf=elf  
	
endif


# Subprojects
.build-subprojects:


# Subprojects
.clean-subprojects:

# Clean Targets
.clean-conf: ${CLEAN_SUBPROJECTS}
	${RM} -r build/pic24hj128gp502_sd_module
	${RM} -r dist/pic24hj128gp502_sd_module

# Enable dependency checking
.dep.inc: .depcheck-impl

DEPFILES=$(shell mplabwildcard ${POSSIBLE_DEPFILES})
ifneq (${DEPFILES},)
include ${DEPFILES}
endif
