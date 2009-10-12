@ REM
@ REM Old_Instruction Cmd0047c0fi250fo5d0300s00cd0d00.bin
@ REM New_Instruction Cmd0047c0fi250fo5d0300s00cd8d02.bin
@ REM
@ REM 08_Gray V100_P034_60_WD0401_TC.wbf
@ REM 16_Gray V110_Q11_60_TE1601_BTC.wbf
@ REM

@ REM Old_Instruction + 08_Gray
copy Cmd0047c0fi250fo5d0300s00cd0d00.bin /b + V100_P034_60_WD0401_TC.wbf /b Cmd0047c0fi250fo5d0300s00cd0d00_V100_P034_60_WD0401_TC.wbf
@ REM copy Cmd0047c0fi250fo5d0300s00cd0d00_V100_P034_60_WD0401_TC.wbf 08Gray.wbf
@ REM Old_Instruction + 16_Gray
copy Cmd0047c0fi250fo5d0300s00cd0d00.bin /b + V110_Q11_60_TE1601_BTC.wbf /b Cmd0047c0fi250fo5d0300s00cd0d00_V110_Q11_60_TE1601_BTC.wbf
@ REM copy Cmd0047c0fi250fo5d0300s00cd0d00_V110_Q11_60_TE1601_BTC.wbf 16Gray.wbf

@ REM New_Instruction + 08_Gray
copy Cmd0047c0fi250fo5d0300s00cd8d02.bin /b + V100_P034_60_WD0401_TC.wbf /b Cmd0047c0fi250fo5d0300s00cd8d02_V100_P034_60_WD0401_TC.wbf
@ REM copy Cmd0047c0fi250fo5d0300s00cd8d02_V100_P034_60_WD0401_TC.wbf 08Gray.wbf
@ REM New_Instruction + 16_Gray
copy Cmd0047c0fi250fo5d0300s00cd8d02.bin /b + V110_Q11_60_TE1601_BTC.wbf /b Cmd0047c0fi250fo5d0300s00cd8d02_V110_Q11_60_TE1601_BTC.wbf
@ REM copy Cmd0047c0fi250fo5d0300s00cd8d02_V110_Q11_60_TE1601_BTC.wbf 16Gray.wbf