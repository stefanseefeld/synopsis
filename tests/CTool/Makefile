
.PHONY: setup all install clean tidy pack

all install:

setup:  clean
	chmod a+x test_suite
	chmod a+x perl/run_suite
	chmod a+x perl/fixin
	perl perl/fixin perl/fixin
	perl/fixin test_suite
	perl/fixin perl/run_suite
	@cd accept0; /bin/rm -f run_suite; ln -s ../perl/run_suite ./run_suite
	@cd accept1; /bin/rm -f run_suite; ln -s ../perl/run_suite ./run_suite
	@cd gcc; /bin/rm -f run_suite; ln -s ../perl/run_suite ./run_suite
	@cd reject0; /bin/rm -f run_suite; ln -s ../perl/run_suite ./run_suite
	@cd reject1; /bin/rm -f run_suite; ln -s ../perl/run_suite ./run_suite

tidy clean:
	@cd accept0; /bin/rm -rf diff
	@cd accept1; /bin/rm -rf diff
	@cd gcc; /bin/rm -rf diff
	@cd reject0; /bin/rm -rf diff
	@cd reject1; /bin/rm -rf diff
	@/bin/rm -f *.bak perl/*.bak

pack: clean
	@cd accept0; /bin/rm -f run_suite
	@cd accept1; /bin/rm -f run_suite
	@cd gcc; /bin/rm -f run_suite
	@cd reject0; /bin/rm -f run_suite
	@cd reject1; /bin/rm -f run_suite
