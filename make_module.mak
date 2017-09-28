define make_module

# reads and then clears $(MODULE), $(LIBS), $(TESTS), $(TEST-DEPS), $(BINS), $(BIN-DEPS)

$(MODULE)-DIR-PREFIX :=
ifneq ($(strip $(MODULE)),)
	$(MODULE)-DIR-PREFIX += $(MODULE)/
endif
$(MODULE)-DIR-PREFIX := $$(strip $$($(MODULE)-DIR-PREFIX))


$(MODULE)-LIB-OBJECTS := $$(addsuffix .o, $$(addprefix $$(OBJDIR)/$$($(MODULE)-DIR-PREFIX), $(LIBS)))
$(MODULE)-TEST-OBJECTS := $$(addsuffix .o, $$(addprefix $$(OBJDIR)/$$($(MODULE)-DIR-PREFIX), $(TESTS)))
$(MODULE)-TEST-BINS := $$(addprefix $$(TESTDIR)/$$($(MODULE)-DIR-PREFIX), $(TESTS))
$(MODULE)-BIN-OBJECTS := $$(addsuffix .o, $$(addprefix $$(OBJDIR)/$$($(MODULE)-DIR-PREFIX), $(BINS)))
$(MODULE)-BINS := $$(addprefix $$(BINDIR)/$$($(MODULE)-DIR-PREFIX), $(BINS))


ifneq ($(strip $(MODULE)),)

$(MODULE)-STATIC-LIBS = $$(OBJDIR)/$(MODULE).a

$$(OBJDIR)/$(MODULE).a: $$($(MODULE)-LIB-OBJECTS)
	@mkdir -p $$(OBJDIR)
	@$$(call build_and_check, $$(AR) $$@ $$^)

endif


OBJECTS += $$($(MODULE)-LIB-OBJECTS) $$($(MODULE)-TEST-OBJECTS) $$($(MODULE)-BIN-OBJECTS)
TEST-BINS += $$($(MODULE)-TEST-BINS)
BINARIES += $$($(MODULE)-BINS)
STATIC-LIBS += $$($(MODULE)-STATIC-LIBS)

$$($(MODULE)-TEST-BINS): $$(TESTDIR)/% : $$(OBJDIR)/%.o $$($(MODULE)-STATIC-LIBS) $(TEST-DEPS)
	@mkdir -p $$(dir $$@)
	@$$(call build_and_check, $$(LD) $$(LDFLAGS) $$^ -o $$@);

$$($(MODULE)-BINS): $$(TESTDIR)/% : $$(OBJDIR)/%.o $$($(MODULE)-STATIC-LIBS) $(BIN-DEPS)
	@mkdir -p $$(dir $$@)
	@$$(call build_and_check, $$(LD) $$(LDFLAGS) $$^ -o $$@);


MODULE :=
LIBS :=
TESTS :=
TEST-DEPS :=
BINS :=
BIN-DEPS :=

endef

$(eval $(call make_module))
