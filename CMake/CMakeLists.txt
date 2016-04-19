CMAKE_MINIMUM_REQUIRED(VERSION 2.6)
PROJECT(call-ui C)

SET(PROJECT_ROOT_DIR "${CMAKE_SOURCE_DIR}/..")
SET(PROJECT_FILES_DIR "${PROJECT_ROOT_DIR}")

SET(PKG_NAME ${CMAKE_PKG_NAME})
SET(BIN_DIR ${CMAKE_BIN_DIR})
SET(RES_DIR ${CMAKE_RES_DIR})

SET(LOCALE_DIR "${RES_DIR}/locale")
SET(EDJ_DIR "${RES_DIR}/edje")
SET(IMG_DIR "${RES_DIR}/images")

SET(INC_DIR "${PROJECT_FILES_DIR}/inc")
SET(EDC_SRC_DIR "${PROJECT_FILES_DIR}/edje_src/edc")
SET(EDC_IMG_DIR "${PROJECT_FILES_DIR}/edje_src/images")
SET(EDJE_DIR "${PROJECT_FILES_DIR}/res/edje")
SET(PO_DIR "${PROJECT_FILES_DIR}/res/po")

SET(EDJE_CC_DEFINITIONS "")

ADD_SUBDIRECTORY(po)

FILE(GLOB SRCS "${PROJECT_FILES_DIR}/src/*.c")

INCLUDE_DIRECTORIES(${PROJECT_FILES_DIR}/inc)

INCLUDE(FindPkgConfig)
pkg_check_modules (pkgs REQUIRED
	appcore-efl
	glib-2.0
	gio-2.0
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
		COMMAND edje_cc -id ${EDC_IMG_DIR}
		${EDJE_CC_DEFINITIONS}
		${EDJE_DIR}/call.edc ${CMAKE_BINARY_DIR}/call.edj
		DEPENDS ${EDJE_DIR}/call.edc
			${INC_DIR}/callui-view-layout.h
			${INC_DIR}/callui-view-caller-info-defines.h
			${INC_DIR}/callui-view-elements-defines.h
			${EDC_SRC_DIR}/callui-color-classes.edc
			${EDC_SRC_DIR}/call_edc/edc_image_macro.edc
			${EDC_SRC_DIR}/call_edc/callui-view-button-layout.edc
			${EDC_SRC_DIR}/call_edc/callui-view-caller-info.edc
			${EDC_SRC_DIR}/call_edc/callui-view-incoming-lock.edc
			${EDC_SRC_DIR}/call_edc/callui-view-multi-call-list.edc
			${EDC_SRC_DIR}/call_edc/callui-keypad.edc
			${EDC_SRC_DIR}/call_edc/callui-view-main-layout.edc
			${EDC_SRC_DIR}/call_edc/callui_view_end_call.edc
			${EDC_SRC_DIR}/call_edc/callui-view-lock-screen.edc
			${EDC_SRC_DIR}/call_edc/callui-view-quick-pannel.edc
			${EDC_SRC_DIR}/call_edc/callui_view_incoming_active.edc
			${EDC_SRC_DIR}/call_edc/callui-view-one-hold-in-conference.edc
			${EDC_SRC_DIR}/call_edc/callui-view-elements.edc
)
ADD_CUSTOM_TARGET(call.edje ALL DEPENDS ${CMAKE_BINARY_DIR}/call.edj)

ADD_CUSTOM_COMMAND(
		OUTPUT ${CMAKE_BINARY_DIR}/call_theme.edj
		COMMAND edje_cc -id ${EDC_IMG_DIR}
		${EDJE_CC_DEFINITIONS}
		${EDJE_DIR}/call_theme.edc ${CMAKE_BINARY_DIR}/call_theme.edj
		DEPENDS ${EDJE_DIR}/call_theme.edc
			${INC_DIR}/callui-view-layout.h
			${EDC_SRC_DIR}/callui-color-classes.edc
)
ADD_CUSTOM_TARGET(call_theme.edje ALL DEPENDS ${CMAKE_BINARY_DIR}/call_theme.edj)

#install binary & edj files
INSTALL(FILES ${CMAKE_BINARY_DIR}/call.edj DESTINATION ${EDJ_DIR})
INSTALL(FILES ${CMAKE_BINARY_DIR}/call_theme.edj DESTINATION ${EDJ_DIR})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BIN_DIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})
INSTALL(FILES ${PROJECT_FILES_DIR}/${PKG_NAME}.xml DESTINATION ${CMAKE_SHARE_PACKAGES_DIR})

# install manifest file & icon
INSTALL(FILES ${PROJECT_FILES_DIR}/res/images/call_button_icon_01.png DESTINATION ${IMG_DIR})
INSTALL(FILES ${PROJECT_FILES_DIR}/res/images/call_button_icon_03.png DESTINATION ${IMG_DIR})
INSTALL(FILES ${PROJECT_FILES_DIR}/res/images/call_button_icon_04.png DESTINATION ${IMG_DIR})

