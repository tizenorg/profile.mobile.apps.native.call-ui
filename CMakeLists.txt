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

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/include)

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
)

FOREACH(flag ${pkgs_CFLAGS})
	SET(EXTRA_CFLAGS "${EXTRA_CFLAGS} ${flag}")
ENDFOREACH(flag)

SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIE ${EXTRA_CFLAGS}")

ADD_EXECUTABLE(${PROJECT_NAME} ${SRCS})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} "-pie" ${pkgs_LDFLAGS})

ADD_CUSTOM_TARGET(call.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/res/images ${SHARED_FLAGS} ${EDJE_CC_DEFINITIONS}
		${CMAKE_CURRENT_SOURCE_DIR}/res/call_edc/call.edc ${CMAKE_CURRENT_SOURCE_DIR}/res/call.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/call_edc/call.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} call.edj)

ADD_CUSTOM_TARGET(call_theme.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/res/themes ${EDJE_CC_DEFINITIONS} -sd
		${CMAKE_CURRENT_SOURCE_DIR}/res/themes/sounds
		${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_theme.edc ${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} call_theme.edj)

ADD_CUSTOM_TARGET(call_genlist_theme.edj
		COMMAND edje_cc -id ${CMAKE_CURRENT_SOURCE_DIR}/res/themes -sd
		${CMAKE_CURRENT_SOURCE_DIR}/res/themes/sounds
		${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_genlist_theme.edc ${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_genlist_theme.edj
		DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_genlist_theme.edc
)
ADD_DEPENDENCIES(${PROJECT_NAME} call_genlist_theme.edj)

#install binary & edj files
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/call.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_theme.edj DESTINATION ${EDJDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/themes/call_genlist_theme.edj DESTINATION ${EDJDIR})

INSTALL(TARGETS ${PROJECT_NAME} DESTINATION ${BINDIR})
INSTALL(DIRECTORY DESTINATION ${DATADIR})
INSTALL(FILES ${CMAKE_SOURCE_DIR}/${PKG_NAME}.xml DESTINATION ${CMAKE_SHARE_PACKAGES_DIR})

# install manifest file & icon
INSTALL(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/res/tables/ DESTINATION ${TABLEDIR} FILES_MATCHING PATTERN "*.xml")
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/handler_btn_02.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/call_photo_id_mask.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/icon/call_resume_play_mask.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/call_photo_id.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/button_edit_bg_new.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/call_button_icon_01.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/call_button_icon_03.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/call_button_icon_04.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/icon/call_button_add.png DESTINATION ${IMGDIR})
INSTALL(FILES ${CMAKE_CURRENT_SOURCE_DIR}/res/images/icon/noti_icon_reject_auto.png DESTINATION ${IMGDIR})

