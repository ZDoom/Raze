#pragma once

BEGIN_BLD_NS

class DBloodActor;
void FireballSeqCallback(int, DBloodActor*);
void sub_38938(int, DBloodActor*);
void NapalmSeqCallback(int, DBloodActor*);
void sub_3888C(int, DBloodActor*);
void TreeToGibCallback(int, DBloodActor*);
void DudeToGibCallback1(int, DBloodActor*);
void DudeToGibCallback2(int, DBloodActor*);
void batBiteSeqCallback(int, DBloodActor*);
void SlashSeqCallback(int, DBloodActor*);
void StompSeqCallback(int, DBloodActor*);
void eelBiteSeqCallback(int, DBloodActor*);
void BurnSeqCallback(int, DBloodActor*);
void SeqAttackCallback(int, DBloodActor*);
void cerberusBiteSeqCallback(int, DBloodActor*);
void cerberusBurnSeqCallback(int, DBloodActor*);
void cerberusBurnSeqCallback2(int, DBloodActor*);
void TommySeqCallback(int, DBloodActor*);
void TeslaSeqCallback(int, DBloodActor*);
void ShotSeqCallback(int, DBloodActor*);
void cultThrowSeqCallback(int, DBloodActor*);
void sub_68170(int, DBloodActor*);
void sub_68230(int, DBloodActor*);
void SlashFSeqCallback(int, DBloodActor*);
void ThrowFSeqCallback(int, DBloodActor*);
void BlastSSeqCallback(int, DBloodActor*);
void ThrowSSeqCallback(int, DBloodActor*);
void ghostSlashSeqCallback(int, DBloodActor*);
void ghostThrowSeqCallback(int, DBloodActor*);
void ghostBlastSeqCallback(int, DBloodActor*);
void GillBiteSeqCallback(int, DBloodActor*);
void HandJumpSeqCallback(int, DBloodActor*);
void houndBiteSeqCallback(int, DBloodActor*);
void houndBurnSeqCallback(int, DBloodActor*);
void sub_6FF08(int, DBloodActor*);
void sub_6FF54(int, DBloodActor*);
void podAttack(int, DBloodActor*);
void sub_70284(int, DBloodActor*);
void ratBiteSeqCallback(int, DBloodActor*);
void SpidBiteSeqCallback(int, DBloodActor*);
void SpidJumpSeqCallback(int, DBloodActor*);
void sub_71370(int, DBloodActor*);
void sub_71A90(int, DBloodActor*);
void sub_71BD4(int, DBloodActor*);
void sub_720AC(int, DBloodActor*);
void genDudeAttack1(int, DBloodActor*);
void punchCallback(int, DBloodActor*);
void ThrowCallback1(int, DBloodActor*);
void ThrowCallback2(int, DBloodActor*);
void HackSeqCallback(int, DBloodActor*);
void StandSeqCallback(int, DBloodActor*);
void zombfHackSeqCallback(int, DBloodActor*);
void PukeSeqCallback(int, DBloodActor*);
void ThrowSeqCallback(int, DBloodActor*);
void PlayerSurvive(int, DBloodActor*);
void PlayerKneelsOver(int, DBloodActor*);
void FireballTrapSeqCallback(int, DBloodActor*);
void MGunFireSeqCallback(int, DBloodActor*);
void MGunOpenSeqCallback(int, DBloodActor*);

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
	nPodStartChaseClient,
	nTentacleStartSearchClient,
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
