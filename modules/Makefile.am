ACLOCAL_AMFLAGS = -I ../acinclude

include $(top_srcdir)/build/common.mk

pkglibdir = $(libdir)

AM_LIBADD = $(top_builddir)/engine/libironbee.la

if FREEBSD
AM_LDFLAGS += -L/usr/local/lib -module -avoid-version -lssp_nonshared
else
AM_LDFLAGS += -module -avoid-version
endif

pkglib_LTLIBRARIES = ibmod_htp.la \
                     ibmod_pcre.la \
                     ibmod_ac.la \
                     ibmod_rules.la \
                     ibmod_user_agent.la

if ENABLE_LUA
pkglib_LTLIBRARIES += ibmod_lua.la
endif

if BUILD_GEOIP
pkglib_LTLIBRARIES += ibmod_geoip.la
ibmod_geoip_la_SOURCES = geoip.c
ibmod_geoip_la_LIBADD = $(AM_LIBADD) -lGeoIP -lm
ibmod_geoip_la_LDFLAGS = $(AM_LDFLAGS) $(GEOIP_LDFLAGS)
ibmod_geoip_la_CFLAGS = $(AM_CFLAGS) $(GEOIP_CFLAGS)
ibmod_geoip_la_CPPFLAGS = $(AM_CPPFLAGS) $(GEOIP_CPPFLAGS)
endif

ibmod_htp_la_SOURCES = modhtp.c
ibmod_htp_la_LDFLAGS = $(AM_LDFLAGS)
ibmod_htp_la_LIBADD = $(AM_LIBADD) \
                      $(top_builddir)/libs/libhtp/htp/libhtp.la
ibmod_htp_la_CPPFLAGS = $(AM_CPPFLAGS) -I$(top_srcdir)/libs/libhtp/htp
ibmod_htp_la_CFLAGS = $(AM_CFLAGS)

ibmod_pcre_la_SOURCES = pcre.c
ibmod_pcre_la_CPPFLAGS = $(AM_CPPFLAGS) @PCRE_CPPFLAGS@
ibmod_pcre_la_CFLAGS = @PCRE_CFLAGS@
ibmod_pcre_la_LDFLAGS = $(AM_LDFLAGS) \
                        @PCRE_LDFLAGS@
ibmod_pcre_la_LIBADD = $(AM_LIBADD) @PCRE_LDADD@

ibmod_ac_la_SOURCES = ac.c
ibmod_ac_la_CFLAGS = ${AM_CFLAGS}
ibmod_ac_la_LDFLAGS = $(AM_LDFLAGS)

pkglib_LTLIBRARIES += ibmod_ee.la
ibmod_ee_la_SOURCES = ee_oper.c
ibmod_ee_la_LIBADD = $(AM_LIBADD) $(top_builddir)/automata/libiaeudoxus.la
ibmod_ee_la_CFLAGS = ${AM_CFLAGS} -I$(top_srcdir)/automata/include
ibmod_ee_la_LDFLAGS = $(AM_LDFLAGS)

if ENABLE_LUA
ibmod_lua_la_SOURCES = lua.c \
                       lua_common.c \
                       lua_common_private.h
ibmod_lua_la_CPPFLAGS = $(AM_CPPFLAGS) \
                        -I$(top_srcdir)/libs/luajit-2.0-ironbee/src -I$(top_srcdir)
ibmod_lua_la_LIBADD = $(AM_LIBADD) \
                      $(top_builddir)/lua/libironbee-lua.la \
                      -L$(abs_top_builddir)/libs/luajit-2.0-ironbee/src \
                      -lluajit-ironbee \
                      -lm
endif

ibmod_rules_la_SOURCES = rules.c
ibmod_rules_la_CPPFLAGS = $(AM_CPPFLAGS) \
                        -I$(top_srcdir) \
                        -D_XOPEN_SOURCE=1

ibmod_rules_la_LIBADD = $(AM_LIBADD) -lm

if ENABLE_LUA
ibmod_rules_la_SOURCES += \
    lua_common.c \
    lua_common_private.h

ibmod_rules_la_CPPFLAGS += \
	$(AM_CPPFLAGS) \
    -I$(top_srcdir)/libs/luajit-2.0-ironbee/src
ibmod_rules_la_LIBADD += -lluajit-ironbee \
                        -L$(abs_top_builddir)/libs/luajit-2.0-ironbee/src \
                         $(top_builddir)/lua/libironbee-lua.la
endif

if ENABLE_JSON
pkglib_LTLIBRARIES += ibmod_persist.la
ibmod_persist_la_SOURCES = persist.c
endif

if BUILD_DEV_MODULES
pkglib_LTLIBRARIES += ibmod_fields.la
ibmod_fields_la_SOURCES = fields.c
ibmod_fields_la_CFLAGS = ${AM_CFLAGS}
ibmod_fields_la_LDFLAGS = $(AM_LDFLAGS)
endif

if BUILD_DEV_MODULES
pkglib_LTLIBRARIES += ibmod_rule_dev.la
ibmod_rule_dev_la_SOURCES = rule_dev.c
ibmod_rule_dev_la_CFLAGS = ${AM_CFLAGS}
ibmod_rule_dev_la_LDFLAGS = $(AM_LDFLAGS)
endif

ibmod_user_agent_la_SOURCES = user_agent.c \
                              user_agent_rules.c \
                              user_agent_private.h
ibmod_user_agent_la_CFLAGS = ${AM_CFLAGS}
ibmod_user_agent_la_LDFLAGS = $(AM_LDFLAGS)
if FREEBSD
ibmod_user_agent_la_LIBADD = $(AM_LIBADD) -liconv
endif

install-exec-hook: $(pkglib_LTLIBRARIES)
	@echo "Removing unused static libraries..."; \
	for m in $(pkglib_LTLIBRARIES); do \
	  base=`echo $$m | sed 's/\..*//'`; \
	  rm -f $(DESTDIR)$(pkglibdir)/$$base.*a; \
	done

uninstall-local: $(pkglib_LTLIBRARIES)
	@echo "Uninstalling Modules..."; \
	for m in $(pkglib_LTLIBRARIES); do \
	  base=`echo $$m | sed 's/\..*//'`; \
	  rm -f $(DESTDIR)$(pkglibdir)/$$base.*so; \
	done

if ENABLE_LUA
all-local: $(abs_top_builddir)/lua/ironbee-ffi-h.lua $(abs_top_srcdir)/lua/ironbee-ffi.lua $(abs_top_srcdir)/lua/ironbee-api.lua $(abs_top_srcdir)/lua/ironbee-modlua.lua
	mkdir -p $(abs_top_builddir)/modules/.libs
	for f in $(+); do \
		cp "$$f" $(abs_top_builddir)/modules/.libs/`basename "$$f"` ; \
	done
endif
