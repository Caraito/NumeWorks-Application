# make        -> output/outils.nwa  (à installer depuis https://my.numworks.com/apps)
# make test   -> vérifie les calculs sur l'ordinateur
# make sim    -> output/outils.nwb  (simulateur Epsilon : epsilon.bin --nwb output/outils.nwb)
NWLINK = npx --yes -- nwlink@1.0.0
SRC = $(wildcard src/*.c)
CFLAGS = -std=c99 -Wall -Wextra -Wno-unused-parameter -Wno-format-zero-length -Wno-format-truncation
SIM_CFLAGS = $(shell $(NWLINK) eadk-cflags-simulator)

output/outils.nwa: $(SRC) src/ui.h output/icon.o
	arm-none-eabi-gcc $(CFLAGS) $(shell $(NWLINK) eadk-cflags-device) -Os \
	  -fdata-sections -ffunction-sections -flto -fno-fat-lto-objects -fwhole-program -fvisibility=internal \
	  $(SRC) output/icon.o $(shell $(NWLINK) eadk-ldflags-device) --specs=nano.specs -Wl,-u,_printf_float \
	  -Wl,-e,main -Wl,-u,eadk_app_name -Wl,-u,eadk_app_icon -Wl,-u,eadk_api_level -Wl,--gc-sections \
	  -flinker-output=nolto-rel -lm -o $@
	arm-none-eabi-strip --strip-debug $@ # infos de débogage de la libc : inutiles et lourdes
	$(NWLINK) nwa-elf $@ output/outils.elf # échoue si un symbole manque sur la calculatrice

output/icon.o: src/icon.png
	mkdir -p output
	$(NWLINK) png-icon-o $< $@

test: $(SRC) src/ui.h tests/test.c
	mkdir -p output
	gcc $(CFLAGS) $(SIM_CFLAGS) -Isrc -g -fsanitize=address,undefined tests/test.c $(filter-out src/main.c,$(SRC)) -lm -o output/test
	./output/test

sim: $(SRC) src/ui.h
	mkdir -p output
	gcc $(CFLAGS) $(SIM_CFLAGS) -O0 -g $(SRC) -shared -lm -o output/outils.nwb

clean:
	rm -rf output

.PHONY: test sim clean
