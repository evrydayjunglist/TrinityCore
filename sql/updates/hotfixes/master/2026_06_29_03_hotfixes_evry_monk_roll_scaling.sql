-- Monk Roll (107427 / 109131) — retail-like aura 373/305 wire scaling (Capture MR-A 2026-06-29)
-- DB2 68275 raw bp 175/275 under-shoots retail wire (~275 lvl 4, ~282 lvl 80 TC vs retail sniff).
-- Base + RealPointsPerLevel tuned to sniffs: 275 + (level - 2) * (6.67/78) ≈ 281.67 at 80; 375 + same on 305.
UPDATE `spell_effect` SET `EffectBasePoints`=275, `EffectRealPointsPerLevel`=0.08551282051282051 WHERE `ID` IN (118195, 120480);
UPDATE `spell_effect` SET `EffectBasePoints`=375, `EffectRealPointsPerLevel`=0.08551282051282051 WHERE `ID` IN (157593, 157592);
