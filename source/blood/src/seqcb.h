#pragma once

BEGIN_BLD_NS

void FireballSeqCallback(int, int);
void sub_38938(int, int);
void NapalmSeqCallback(int, int);
void sub_3888C(int, int);
void TreeToGibCallback(int, int);
void DudeToGibCallback1(int, int);
void DudeToGibCallback2(int, int);
void batBiteSeqCallback(int, int);
void SlashSeqCallback(int, int);
void StompSeqCallback(int, int);
void eelBiteSeqCallback(int, int);
void BurnSeqCallback(int, int);
void SeqAttackCallback(int, int);
void cerberusBiteSeqCallback(int, int);
void cerberusBurnSeqCallback(int, int);
void cerberusBurnSeqCallback2(int, int);
void TommySeqCallback(int, int);
void TeslaSeqCallback(int, int);
void ShotSeqCallback(int, int);
void cultThrowSeqCallback(int, int);
void sub_68170(int, int);
void sub_68230(int, int);
void SlashFSeqCallback(int, int);
void ThrowFSeqCallback(int, int);
void BlastSSeqCallback(int, int);
void ThrowSSeqCallback(int, int);
void ghostSlashSeqCallback(int, int);
void ghostThrowSeqCallback(int, int);
void ghostBlastSeqCallback(int, int);
void GillBiteSeqCallback(int, int);
void HandJumpSeqCallback(int, int);
void houndBiteSeqCallback(int, int);
void houndBurnSeqCallback(int, int);
void sub_6FF08(int, int);
void sub_6FF54(int, int);
void sub_6FFA0(int, int);
void sub_70284(int, int);
void ratBiteSeqCallback(int, int);
void SpidBiteSeqCallback(int, int);
void SpidJumpSeqCallback(int, int);
void sub_71370(int, int);
void sub_71A90(int, int);
void sub_71BD4(int, int);
void sub_720AC(int, int);
void genDudeAttack1(int, int);
void punchCallback(int, int);
void ThrowCallback1(int, int);
void ThrowCallback2(int, int);
void HackSeqCallback(int, int);
void StandSeqCallback(int, int);
void zombfHackSeqCallback(int, int);
void PukeSeqCallback(int, int);
void ThrowSeqCallback(int, int);
void PlayerSurvive(int, int);
void PlayerKneelsOver(int, int);
void FireballTrapSeqCallback(int, int);
void MGunFireSeqCallback(int, int);
void MGunOpenSeqCallback(int, int);

enum
{
	nFireballClient,
	dword_2192D8,
	nNapalmClient,
	dword_2192E0,
	nTreeToGibClient,
	nDudeToGibClient1,
	nDudeToGibClient2,
	nBatBiteClient,
	nSlashClient,
	nStompClient,
	nEelBiteClient,
	nBurnClient,
	nAttackClient,
	nCerberusBiteClient,
	nCerberusBurnClient,
	nCerberusBurnClient2,
	nTommyClient,
	nTeslaClient,
	nShotClient,
	nThrowClient,
	n68170Client,
	n68230Client,
	nSlashFClient,
	nThrowFClient,
	nThrowSClient,
	nBlastSClient,
	nGhostSlashClient,
	nGhostThrowClient,
	nGhostBlastClient,
	nGillBiteClient,
	nJumpClient,
	nHoundBiteClient,
	nHoundBurnClient,
	dword_279B34,
	dword_279B38,
	dword_279B3C,
	dword_279B40,
	nRatBiteClient,
	nSpidBiteClient,
	nSpidJumpClient,
	dword_279B50,
	dword_279B54,
	dword_279B58,
	dword_279B5C,
	nGenDudeAttack1,
	nGenDudePunch,
	nGenDudeThrow1,
	nGenDudeThrow2,
	nHackClient,
	nStandClient,
	nZombfHackClient,
	nZombfPukeClient,
	nZombfThrowClient,
	nPlayerSurviveClient,
	nPlayerKneelClient,
	nFireballTrapClient,
	nMGunFireClient,
	nMGunOpenClient,
};


END_BLD_NS
