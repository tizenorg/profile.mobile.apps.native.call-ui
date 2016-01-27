CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(call-ui C)

SET(PKG_NAME ${CMAKE_PKG_NAME})
SET(APPDIR ${CMAKE_APP_DIR})
SET(BINDIR ${CMAKE_BIN_DIR})
SET(RESDIR ${CMAKE_RES_DIR})
SET(LOCALEDIR "${RESDIR}/locale")
SET(EDJDIR "${RESDIR}/edje")
SET(IMGDIR "${RESDIR}/images")

SET(INCDIR "${CMAKE_SOURCE_DIR}/inc")
SET(EDCSRCDIR "${CMAKE_SOURCE_DIR}/res/edje_src/edc")
SET(EDCIMGDIR "${CMAKE_SOURCE_DIR}/res/edje_src/edc_images")
SET(EDJEDIR "${CMAKE_SOURCE_DIR}/res/edje")

SET(EDJE_CC_DEFINITIONS "")

ADD_DEFINITIONS("-DBINDIR=\"${BINDIR}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PKG_NAME}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")
ADD_DEFINITIONS("-DIMGDIR=\"${IMGDIR}\"")

ADD_SUBDIRECTORY(res/po)

FILE(GLOB SRCS "src/*.c")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/inc)

INCLUDE(FindPkgConfig)
pkg_check_modules (pkgs REQUIRED
	appcore-efl
	glib-2.0
	elementary
	dlog
	capi-appfw-application
	ecore
	appsvc
	callmgr_client
	contacts-service2
	minicontrol-provider
	vconf
	capi-network-bluetooth
	capi-network-connection
	capi-system-system-settings
	msg-service
	capi-system-device
	notification
	capi-system-runtime-info
	capi-system-info
	efl-extension
	capi-system-sensor
	capi-ui-efl-util
	libtzplatform-config
)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE ${EXTRA_CFLAGS} -Wall -Werror")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} "-pie" ${pkgs_LDFLAGS})

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_BINARY_DIR}/call.edj
		COMMAND edje_cc -no-save -id ${EDCIMGDIR}
		${EDJE_CC_DEFINITIONS}
		${EDJEDIR}/call.edc ${CMAKE_BINARY_DIR}/call.edj
		DEPENDS ${EDJEDIR}/call.edc
			${INCDIR}/callui-view-layout-wvga.h
			${INCDIR}/callui-view-layout-hd.h
			${INCDIR}/callui-view-caller-info-defines.h
			${INCDIR}/callui-view-elements-defines.h
			${EDCSRCDIR}/callui-color-classes.edc
			${EDCSRCDIR}/call_edc/edc_image_macro.edc
			${EDCSRCDIR}/call_edc/callui-view-button-layout.edc
			${EDCSRCDIR}/call_edc/callui-view-caller-info.edc
			${EDCSRCDIR}/call_edc/callui-view-incoming-lock.edc
			${EDCSRCDIR}/call_edc/callui-view-multi-call-list.edc
			${EDCSRCDIR}/call_edc/callui-keypad.edc
			${EDCSRCDIR}/call_edc/callui-view-main-layout.edc
			${EDCSRCDIR}/call_edc/callui_view_end_call.edc
			${EDCSRCDIR}/call_edc/callui-view-lock-screen.edc
			${EDCSRCDIR}/call_edc/callui-view-quick-pannel.edc
			${EDCSRCDIR}/call_edc/callui-view-quick-pannel-landscape.edc
			${EDCSRCDIR}/call_edc/callui_view_incoming_active.edc
			${EDCSRCDIR}/call_edc/callui-view-one-hold-in-conference.edc
			${EDCSRCDIR}/call_edc/callui-view-elements.edc
)
ADD_CUSTOM_TARGET(call.edje ALL DEPENDS ${CMAKE_BINARY_DIR}/call.edj)

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_BINARY_DIR}/call_theme.edj
		COMMAND edje_cc -no-save -id ${EDCIMGDIR}
		${EDJE_CC_DEFINITIONS}
		${EDJEDIR}/call_theme.edc ${CMAKE_BINARY_DIR}/call_theme.edj
		DEPENDS ${EDJEDIR}/call_theme.edc
			${INCDIR}/callui-view-layout-wvga.h
			${EDCSRCDIR}/callui-color-classes.edc
)
ADD_CUSTOM_TARGET(call_theme.edje ALL DEPENDS ${CMAKE_BINARY_DIR}/call_theme.edj)

#install binary & edj files
INSTALL(FILES ${CMAKE_BINARY_DIR}/call.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/call_theme.edj DESTINATION ${EDJDIR})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/${PKG_NAME}.xml DESTINATION ${CMAKE_SHARE_PACKAGES_DIR})

# install manifest file & icon
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/handler_btn_02.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_photo_id_mask.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_resume_play_mask.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_photo_id.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/button_edit_bg_new.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_button_icon_01.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_button_icon_03.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_button_icon_04.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_button_add.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/noti_icon_reject_auto.png DESTINATION ${IMGDIR})
