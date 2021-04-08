const void loadCode(const char *);

// ********************************************
// * HERE is where you load your default code *
// ********************************************

void loadBaseSystem() {
    loadCode("{MB 0(p--) :SC :RD :CQ (:RV :RA)}");
    loadCode("{SC 0(p--) p! p@ 100+ m! m@ M@ s! 0(todo xxx r!)}");
    loadCode("{RD 0(--)  p@ DR v!}");
    loadCode("{CQ 0(--f) m@ M@ v@ =}");
    loadCode("{RV 0(--)  v@ m@ M!}");
    loadCode("{RA 0(--)  todo}");
}
