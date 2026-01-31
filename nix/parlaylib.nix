{ stdenv, lib, fetchgit, cmake,
  parlaySrc ? fetchgit {
    url = "https://github.com/mikerainey/parlaylib.git";
    rev = "9bb91b2613f2644a879242215b0d525813c5131b";
    sha256 = "sha256-YGIB922DD0XZ36gOEyVpAfiJ3bfBSw+dxe0nuV0SiZc=";
  },
  parlaySequential ? false, taskparts ? null, parlayCilkPlus ? false, parlayOpenCilk ? false, parlayOpenMP ? false,
  parlayExamples ? false, parlayInstallExamples ? false, parlayExampleData ? false, parlayFewExamples ? false,
  parlayDisableElasticParallelism ? false,
  gbenchmarkSrc ? null,
  gtestSrc ? null
}:

stdenv.mkDerivation rec {

  name = "parlaylib";

  src = parlaySrc;

  nativeBuildInputs = [ cmake ]
                      ++ (if taskparts != null then [ taskparts ] else []);

  cmakeFlags = [
    (lib.strings.optionalString parlayExamples "-DPARLAY_EXAMPLES=ON")
    (lib.strings.optionalString parlayInstallExamples "-DPARLAY_INSTALL_EXAMPLES=ON")
    (lib.strings.optionalString parlayExampleData "-DPARLAY_EXAMPLE_DATA=ON")
    (lib.strings.optionalString parlayFewExamples "-DFEW_EXAMPLES=ON")
    (lib.strings.optionalString parlayDisableElasticParallelism "-DPARLAY_ELASTIC_PARALLELISM=OFF")
    (lib.strings.optionalString parlaySequential "-DPARLAY_SEQUENTIAL=ON")
    (lib.strings.optionalString (taskparts != null) "-DPARLAY_TASKPARTS=ON")
    (lib.strings.optionalString parlayCilkPlus "-DPARLAY_CILKPLUS=ON")
    (lib.strings.optionalString parlayOpenCilk "-DPARLAY_OPENCILK=ON")
    (lib.strings.optionalString parlayOpenMP "-DPARLAY_OPENMP=ON")
    (lib.strings.optionalString (gbenchmarkSrc != null) "-DFETCHCONTENT_SOURCE_DIR_BENCHMARK=${gbenchmarkSrc}")
    (lib.strings.optionalString (gtestSrc != null) "-DCMAKE_BUILD_TYPE=Debug")
    (lib.strings.optionalString (gtestSrc != null) "-DFETCHCONTENT_SOURCE_DIR_GOOGLETEST=${gtestSrc}")
  ];

  meta = {
    description = "A Toolkit for Programming Parallel Algorithms on Shared-Memory Multicore Machines";
    license = "MIT";
    homepage = https://cmuparlay.github.io/parlaylib/;
  };

}
