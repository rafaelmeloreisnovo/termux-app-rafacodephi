/* Authorial, freestanding, hosted-vector compatible semantic falsifier. */
#include "../rafaelia/src/main/cpp/zero/rafz_ninja_l0.h"
#define TEXT_LEN(s) ((unsigned)(sizeof(s)-1u))
static struct rafn_l0_plan plan;
static unsigned checks;
static int expect(const char *s,unsigned n,int expected) {
    int got=rafn_l0_parse(s,n,&plan);
    ++checks;
    if(got!=expected) return (int)checks;
    /* On any failure, the plan must be unambiguously empty even when a
     * previous successful parse or partial nodes were present. */
    if(got!=RAFN_L0_OK &&
       (plan.rule_count || plan.node_count || plan.order_count ||
        plan.has_default || plan.rules[0].name[0] || plan.nodes[0].output[0] ||
        plan.mark[0] || plan.default_target[0])) return (int)(checks+64u);
    return 0;
}
int main(void) {
    int e;
    static const char dag[] =
        "rule cc\n  command = clang -c main.c -o main.o\n"
        "rule ld\n  command = clang main.o -o app\n"
        "build app: ld main.o\n"
        "build main.o: cc main.c\n"
        "default app\n";
    static const char forward[] =
        "build bundle: phony a b\n"
        "build a: phony src\n"
        "build b: phony src\n";
    static const char duplicate[] =
        "build a: phony\nbuild a: phony\n";
    static const char cycle[] =
        "build a: phony b\nbuild b: phony a\n";
    static const char unsupported[] =
        "rule cc\n  command = cc $in -o $out\nbuild a: cc b\n";
    static const char badrule[] = "build a: missing b\n";
    static const char baddefault[] = "build a: phony\ndefault missing\n";
    static const char badpipe[] = "build a: phony b | c\n";
    static const char traversal[] = "build ../out: phony a\n";
    static const char badindent[] = " command = false\n";
    static const char missing_cmd[] = "rule cc\nbuild a: cc b\n";
    static const char empty_graph[] = "# source-only comment\n";
    static const char badruledef[] = "rule a\n  command = true\nrule a\n  command = true\n";
    e=expect(dag,TEXT_LEN(dag),RAFN_L0_OK); if(e) return e;
    if(plan.node_count!=2 || plan.order_count!=2 ||
       !rafn_l0_eq(plan.nodes[plan.order[0]].output,"main.o") ||
       !rafn_l0_eq(plan.nodes[plan.order[1]].output,"app")) return 80;
    e=expect(forward,TEXT_LEN(forward),RAFN_L0_OK); if(e) return e;
    if(plan.order_count!=3 ||
       !rafn_l0_eq(plan.nodes[plan.order[2]].output,"bundle")) return 81;
    e=expect(duplicate,TEXT_LEN(duplicate),RAFN_L0_DUPLICATE); if(e) return e;
    e=expect(cycle,TEXT_LEN(cycle),RAFN_L0_CYCLE); if(e) return e;
    e=expect(unsupported,TEXT_LEN(unsupported),RAFN_L0_SYNTAX); if(e) return e;
    e=expect(badrule,TEXT_LEN(badrule),RAFN_L0_UNDEFINED_RULE); if(e) return e;
    e=expect(baddefault,TEXT_LEN(baddefault),RAFN_L0_UNDEFINED_DEFAULT); if(e) return e;
    e=expect(badpipe,TEXT_LEN(badpipe),RAFN_L0_SYNTAX); if(e) return e;
    e=expect(traversal,TEXT_LEN(traversal),RAFN_L0_SYNTAX); if(e) return e;
    e=expect(badindent,TEXT_LEN(badindent),RAFN_L0_SYNTAX); if(e) return e;
    e=expect(missing_cmd,TEXT_LEN(missing_cmd),RAFN_L0_UNDEFINED_RULE); if(e) return e;
    e=expect(badruledef,TEXT_LEN(badruledef),RAFN_L0_DUPLICATE); if(e) return e;
    e=expect(empty_graph,TEXT_LEN(empty_graph),RAFN_L0_SYNTAX); if(e) return e;
    e=expect((const char *)0,0,RAFN_L0_INVALID); if(e) return e;
    e=expect(dag,RAFN_L0_FILE+1,RAFN_L0_LIMIT); if(e) return e;
    return 0;
}
