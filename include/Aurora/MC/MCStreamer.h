#ifndef AURORA_MC_MCSTREAMER_H
#define AURORA_MC_MCSTREAMER_H

#include <cstdint>
#include <ostream>
#include <string>
#include <vector>

namespace aurora {

class MachineInstr;
class MachineOperand;

class MCStreamer {
public:
    virtual ~MCStreamer() = default;

    virtual void emitInstruction(const MachineInstr& mi) = 0;
    virtual void emitLabel(const std::string& name) = 0;
    virtual void emitComment(const std::string& text) = 0;
    virtual void emitGlobalSymbol(const std::string& name) = 0;
    virtual void emitAlignment(unsigned bytes) = 0;
    virtual void emitBytes(const std::vector<uint8_t>& data) = 0;
    virtual void emitString(const std::string& str) = 0;

    virtual void emitRawText(const std::string& text) = 0;
};

class AsmTextStreamer : public MCStreamer {
public:
    explicit AsmTextStreamer(std::ostream& os);
    ~AsmTextStreamer() override;

    void emitInstruction(const MachineInstr& mi) override;
    void emitLabel(const std::string& name) override;
    void emitComment(const std::string& text) override;
    void emitGlobalSymbol(const std::string& name) override;
    void emitAlignment(unsigned bytes) override;
    void emitBytes(const std::vector<uint8_t>& data) override;
    void emitString(const std::string& str) override;
    void emitRawText(const std::string& text) override;

    std::ostream& getOS() noexcept { return os_; }

private:
    std::ostream& os_;
};

} // namespace aurora

#endif // AURORA_MC_MCSTREAMER_H
