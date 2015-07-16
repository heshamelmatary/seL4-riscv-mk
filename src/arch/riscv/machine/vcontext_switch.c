void riscv_vm_contextSwitch(pde_t* cap_pd)
{
    setCurrentPD(addrFromPPtr(cap_pd));
    //setCurrentASID(cap_pd);
}

/** MODIFIES: [*] */
void setHardwareASID(uint32_t asid)
{
    //writeContextID(hw_asid);
}
