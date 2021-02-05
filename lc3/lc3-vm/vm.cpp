#include "vm.h"

#include "flags.h"
#include "utility.h"
#include "op_codes.h"

#include <iostream>

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <conio.h>
#include <signal.h>

#undef max

enum traps
{
    tr_getc = 0x20,
    tr_out = 0x21,
    tr_puts = 0x22,
    tr_in = 0x23,
    tr_putsp = 0x24,
    tr_halt = 0x25
};

vm::vm()
{
    enable_echo(false);
}

vm::~vm()
{
    enable_echo(true);
}

void vm::run()
{
    pc() = 0x3000;
    running_ = true;
    while (running_)
    {
        auto inst = next_instruction();
        auto op = static_cast<op_codes>(inst >> 12);
        switch (op)
        {
        case op_codes::op_add:
            add(inst);
            break;
        case op_codes::op_and:
            do_and(inst);
            break;
        case op_codes::op_not:
            do_not(inst);
            break;
        case op_codes::op_br:
            br(inst);
            break;
        case op_codes::op_jsr:
            jsr(inst);
            break;
        case op_codes::op_ld:
            ld(inst);
            break;
        case op_codes::op_ldi:
            ldi(inst);
            break;
        case op_codes::op_ldr:
            ldr(inst);
            break;
        case op_codes::op_lea:
            lea(inst);
            break;
        case op_codes::op_st:
            st(inst);
            break;
        case op_codes::op_sti:
            sti(inst);
            break;
        case op_codes::op_str:
            str(inst);
            break;
        case op_codes::op_trap:
            trap(inst);
            break;
        case op_codes::op_jmp:
            jmp(inst);
            break;
        case op_codes::op_res:
        case op_codes::op_rti:
        default:
            std::abort();
            break;
        }
    }
}


void vm::load(std::istream &stream)
{
    uint16_t origin;
    stream.read(reinterpret_cast<char*>(&origin), sizeof(uint16_t));
    origin = flip16(origin);

    uint16_t max_fs = std::numeric_limits<uint16_t>::max() - origin;
    auto loc = memory_.get() + origin;
    stream.read(reinterpret_cast<char *>(loc), max_fs * 2);
    auto rd = stream.gcount();
    while (rd--)
    {
        *loc = flip16(*loc);
        ++loc;
    }
}

uint16_t vm::next_instruction()
{
    auto addr = registers_[registers::pc]++;
    return memory_.read(addr);
}

uint16_t vm::regaddr_at(uint16_t loc, uint16_t inst)
{
    return (inst >> loc) & 0x7;
}

void vm::set_cc(uint16_t reg_addr)
{
    auto v = registers_[reg_addr];
    auto &cond_reg = registers_[registers::cond];
    if (v == 0)
    {
        cond_reg = flags::zero;
    }
    else if ((v & 0x8000) == 0x8000)
    {
        cond_reg = flags::neg;
    }
    else
    {
        cond_reg = flags::pos;
    }
}

uint16_t& vm::pc()
{
    return registers_[registers::pc];
}

void vm::add(uint16_t inst)
{
    auto dra = (inst >> 9) & 0x7;
    auto r1 = (inst >> 6) & 0x7;
    auto imm = (inst >> 5) & 0x1;
    if (imm)
    {
        auto val = sign_extend(inst & 0x1F, 5);
        registers_[dra] = registers_[r1] + val;
    }
    else
    {
        auto r2 = inst & 0x7;
        registers_[dra] = registers_[r1] + registers_[r2];
    }
    set_cc(dra);
}

void vm::do_and(uint16_t inst)
{
    auto dra = (inst >> 9) & 0x7;
    auto sra = (inst >> 6) & 0x7;
    auto imm = (inst >> 5) & 0x01;

    auto &dr = registers_[dra];
    auto sr = registers_[sra];
    if (imm)
    {
        auto imv = sign_extend(inst & 0x1F, 5);
        dr = sr & imv;
    }
    else
    {
        auto sr2a = (inst & 0x7);
        auto sr2 = registers_[sr2a];
        dr = sr & sr2;
    }
    set_cc(dra);
}

void vm::do_not(uint16_t inst)
{
    auto dra = regaddr_at(9, inst);
    auto sra = regaddr_at(6, inst);
    registers_[dra] = ~registers_[sra];
    set_cc(dra);
}

void vm::br(uint16_t inst)
{
    auto offset = sign_extend(inst & 0x1FF, 9);
    auto cond = (inst >> 9) & 0x7;
    auto rv = registers_[registers::cond];
    if (rv&cond)
    {
        pc() = pc() + offset;
    }
}

void vm::ldi(uint16_t inst)
{
    auto r0 = (inst >> 9) & 0x7;
    auto offset = sign_extend(inst & 0x1FF, 9);
    registers_[r0] = memory_.read(memory_.read(registers_[registers::pc] + offset));
    set_cc(r0);
}

void vm::ld(uint16_t inst)
{
    auto offset = sign_extend(inst & 0x00FF, 8);
    auto dra = (inst >> 9) & 0x7;
    registers_[dra] = memory_.read(pc() + offset);
    set_cc(dra);
}

void vm::ldr(uint16_t inst)
{
    auto dra = regaddr_at(9, inst);
    auto base = regaddr_at(6, inst);
    auto offset = sign_extend((inst & 0x1F), 5);
    auto br = registers_[base];
    registers_[dra] = memory_.read(br + offset);
    set_cc(dra);
}

void vm::lea(uint16_t inst)
{
    auto dra = regaddr_at(9, inst);
    auto offset = sign_extend(inst & 0x1FF, 9);
    registers_[dra] = pc() + offset;
    set_cc(dra);
}

void vm::jmp(uint16_t inst)
{
    auto ra = (inst >> 6) & 0x7;
    auto rsv = registers_[ra];
    pc() = rsv;
}

void vm::jsr(uint16_t inst)
{
    registers_[registers::r7] = pc();
    auto long_jsr = (inst >> 11) & 0x1;
    if (long_jsr)
    {
        auto v = sign_extend(inst & 0x7FF, 11);
        pc() = pc() + v;        
    }
    else
    {
        auto sra = (inst >> 6) & 0x7;
        pc() = registers_[sra];
    }
}

void vm::st(uint16_t inst)
{
    auto offset = sign_extend(inst & 0x1FF, 9);
    auto sra = regaddr_at(9, inst);
    memory_.write(pc() + offset, registers_[sra]);
}

void vm::sti(uint16_t inst)
{
    auto offset = sign_extend(inst & 0x1FF, 9);
    auto sra = regaddr_at(9, inst);
    auto addr = memory_.read(pc() + offset);
    memory_.write(addr, registers_[sra]);
}

void vm::str(uint16_t inst)
{
    auto sra = regaddr_at(9, inst);
    auto bra = regaddr_at(6, inst);
    auto offset = sign_extend(inst & 0x3F, 6);
    auto br = registers_[bra];
    memory_.write(br + offset, registers_[sra]);
}

void vm::trap(uint16_t inst)
{
    switch (inst & 0x00FF)
    {
    case traps::tr_getc:
        getc();
        break;
    case traps::tr_out:
        out();
        break;
    case traps::tr_puts:
        puts();
        break;
    case traps::tr_in:
        in();
        break;
    case traps::tr_putsp:
        putsp();
        break;
    case traps::tr_halt:
        halt();
        break;
    default:
        break;
    }
}

void vm::getc()
{    
    uint16_t c = _getch();
    c = c & 0x00FF;
    registers_[registers::r0] = c;
}

void vm::out()
{
    char c = registers_[registers::r0] & 0x00FF;
    _putch(c);
}

void vm::puts()
{
    auto addr = registers_[registers::r0];
    auto val = memory_.read(addr++);
    while (val != 0)
    {
        _putch(val);
        val = memory_.read(addr++);
    }
}

void vm::putsp()
{
    auto addr = registers_[registers::r0];
    auto val = memory_.read(addr++);
    while (val != 0)
    {
        auto v1 = val & 0x00FF;
        _putch(v1);
        auto v2 = (val >> 8) && 0x00FF;
        if(v2 != 0)
            _putch(v2);
        val = memory_.read(addr++);
    }
}

void vm::in()
{
    std::cout << "Enter a character: ";
    auto v = _getch();
    putc(v, stdout);
    v = v & 0x00FF;
    registers_[registers::r0]=v;
}

void vm::halt()
{
    running_ = false;
    std::cout << "Halted" << std::endl;
}

void vm::enable_echo(bool enable)
{
    HANDLE csl = GetStdHandle(STD_INPUT_HANDLE);
    DWORD mode = { 0 };
    GetConsoleMode(csl, &mode);
    if (enable)
        mode = mode & ENABLE_ECHO_INPUT & ENABLE_LINE_INPUT;
    else
        mode = mode & ~ENABLE_ECHO_INPUT & ~ENABLE_LINE_INPUT;
    SetConsoleMode(csl, mode);
}