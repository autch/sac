
AM_CXXFLAGS = --std=c++11 -fno-exceptions -fno-rtti

bin_PROGRAMS = sac
noinst_LTLIBRARIES = libsac.la

SRC_coder = coder/bufio.h coder/map.cpp coder/map.h coder/range.cpp coder/range.h coder/vle.cpp coder/vle.h
SRC_common = common/histbuf.h common/rand.h common/timer.h
SRC_file = file/file.cpp file/file.h file/sac.cpp file/sac.h file/wav.cpp file/wac.h
SRC_math = math/cholesky.cpp math/cholesky.h math/cov.h math/matrix.h
SRC_model = model/counter.h model/domain.h model/mixer.h model/model.h model/sse.h
SRC_pred = pred/bias.h pred/blend.h pred/lpc.h pred/nlms.h pred/rbf.h pred/rls.cpp pred/rls.h

libsac_la_SOURCES = $(SRC_coder) $(SRC_common) $(SRC_file) $(SRC_math) $(SRC_model) $(SRC_pred) global.h
sac_SOURCES = codec.cpp codec.h global.h main.cpp profile.cpp profile.h utils.h
sac_LDADD = libsac.la
