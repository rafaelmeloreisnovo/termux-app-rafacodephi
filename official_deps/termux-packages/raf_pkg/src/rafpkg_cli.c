#include "rafpkg_common.h"

int rafpkg_plan_print(const char *roots_csv, const char *arch);
int rafpkg_audit_print(const char *roots_csv, const char *arch);
int rafpkg_scan_report(const char *roots_csv, const char *arch, const char *outdir);

static void usage(void){
  fprintf(stderr,
    "rafpkg (RAFAELIA)\n"
    "usage:\n"
    "  rafpkg plan [--roots a,b,c] [--arch ARCH]\n"
    "  rafpkg scan [--roots a,b,c] [--arch ARCH] [--out DIR]\n"
    "  rafpkg audit [--roots a,b,c] [--arch ARCH]\n"
  );
}

static const char *env_arch_default(void){
  const char *a = getenv("TERMUX_ARCH");
  return (a && *a) ? a : "aarch64";
}

int main(int argc, char **argv){
  const char *cmd = (argc>=2)? argv[1] : NULL;
  const char *roots = NULL;
  const char *arch = env_arch_default();
  const char *outdir = NULL;

  for(int i=2;i<argc;i++){
    if(strcmp(argv[i], "--roots")==0 && i+1<argc){ roots = argv[++i]; continue; }
    if(strcmp(argv[i], "--arch")==0 && i+1<argc){ arch = argv[++i]; continue; }
    if(strcmp(argv[i], "--out")==0 && i+1<argc){ outdir = argv[++i]; continue; }
    if(strcmp(argv[i], "-h")==0 || strcmp(argv[i], "--help")==0){ usage(); return 0; }
  }

  if(!cmd){ usage(); return 2; }

  if(strcmp(cmd, "plan")==0)  return rafpkg_plan_print(roots, arch);
  if(strcmp(cmd, "audit")==0) return rafpkg_audit_print(roots, arch);
  if(strcmp(cmd, "scan")==0)  return rafpkg_scan_report(roots, arch, outdir?outdir:".");

  usage();
  return 2;
}
