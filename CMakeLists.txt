CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(call-ui C)

SET(PKG_NAME ${CMAKE_PKG_NAME})
SET(APPDIR ${CMAKE_APP_DIR})
SET(BINDIR ${CMAKE_BIN_DIR})
SET(RESDIR ${CMAKE_RES_DIR})
SET(LOCALEDIR "${RESDIR}/locale")
SET(EDJDIR "${RESDIR}/edje")
SET(IMGDIR "${RESDIR}/images")
SET(TABLEDIR "${RESDIR}/tables")

SET(EDJE_CC_DEFINITIONS "")

ADD_DEFINITIONS("-DBINDIR=\"${BINDIR}\"")
ADD_DEFINITIONS("-DPACKAGE=\"${PKG_NAME}\"")
ADD_DEFINITIONS("-DRESDIR=\"${RESDIR}\"")
ADD_DEFINITIONS("-DLOCALEDIR=\"${LOCALEDIR}\"")
ADD_DEFINITIONS("-DEDJDIR=\"${EDJDIR}\"")
ADD_DEFINITIONS("-DIMGDIR=\"${IMGDIR}\"")
ADD_DEFINITIONS("-DTABLEDIR=\"${TABLEDIR}\"")

ADD_SUBDIRECTORY(po)

FILE(GLOB SRCS "src/*.c")

INCLUDE_DIRECTORIES(${CMAKE_SOURCE_DIR}/include)

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

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE ${EXTRA_CFLAGS}")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} "-pie" ${pkgs_LDFLAGS})

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_BINARY_DIR}/call.edj
		COMMAND edje_cc -no-save -id ${CMAKE_SOURCE_DIR}/res/images
		-id ${CMAKE_SOURCE_DIR} ${EDJE_CC_DEFINITIONS}
		${CMAKE_SOURCE_DIR}/res/call_edc/call.edc ${CMAKE_BINARY_DIR}/call.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/res/call_edc/call.edc
			${CMAKE_SOURCE_DIR}/include/callui-view-layout-wvga.h
			${CMAKE_SOURCE_DIR}/include/callui-view-layout-hd.h
			${CMAKE_SOURCE_DIR}/include/callui-view-caller-info-defines.h
			${CMAKE_SOURCE_DIR}/include/callui-view-elements-defines.h
			${CMAKE_SOURCE_DIR}/res/callui-color-classes.h
			${CMAKE_SOURCE_DIR}/res/call_edc/edc_image_macro.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-button-layout.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-caller-info.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-incoming-lock.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-multi-call-list.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-keypad.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-main-layout.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui_view_end_call.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-lock-screen.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-quick-pannel.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-quick-pannel-landscape.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui_view_incoming_active.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-one-hold-in-conference.edc
			${CMAKE_SOURCE_DIR}/res/call_edc/callui-view-elements.edc
)
ADD_CUSTOM_TARGET(call.edje ALL DEPENDS ${CMAKE_BINARY_DIR}/call.edj)

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_BINARY_DIR}/call_theme.edj
		COMMAND edje_cc -no-save -id ${CMAKE_SOURCE_DIR}/res/themes ${EDJE_CC_DEFINITIONS}
		${CMAKE_SOURCE_DIR}/res/themes/call_theme.edc ${CMAKE_BINARY_DIR}/call_theme.edj
		DEPENDS ${CMAKE_SOURCE_DIR}/res/themes/call_theme.edc
)
ADD_CUSTOM_TARGET(call_theme.edje ALL DEPENDS ${CMAKE_BINARY_DIR}/call_theme.edj)

#install binary & edj files
INSTALL(FILES ${CMAKE_BINARY_DIR}/call.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/call_theme.edj DESTINATION ${EDJDIR})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/${PKG_NAME}.xml DESTINATION ${CMAKE_SHARE_PACKAGES_DIR})

# install manifest file & icon
INSTALL(DIRECTORY ${CMAKE_SOURCE_DIR}/res/tables/ DESTINATION ${TABLEDIR} FILES_MATCHING PATTERN "*.xml")
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/handler_btn_02.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_photo_id_mask.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/icon/call_resume_play_mask.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_photo_id.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/button_edit_bg_new.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_button_icon_01.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_button_icon_03.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/call_button_icon_04.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/icon/call_button_add.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/res/images/icon/noti_icon_reject_auto.png DESTINATION ${IMGDIR})
