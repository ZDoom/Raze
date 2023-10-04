#pragma once

BEGIN_BLD_NS

class DBloodActor;
void FireballSeqCallback(DBloodActor*);
void Fx33Callback(DBloodActor*);
void NapalmSeqCallback(DBloodActor*);
void Fx32Callback(DBloodActor*);
void TreeToGibCallback(DBloodActor*);
void DudeToGibCallback1(DBloodActor*);
void DudeToGibCallback2(DBloodActor*);
void batBiteSeqCallback(DBloodActor*);
void SlashSeqCallback(DBloodActor*);
void StompSeqCallback(DBloodActor*);
void eelBiteSeqCallback(DBloodActor*);
void BurnSeqCallback(DBloodActor*);
void SeqAttackCallback(DBloodActor*);
void cerberusBiteSeqCallback(DBloodActor*);
void cerberusBurnSeqCallback(DBloodActor*);
void cerberusBurnSeqCallback2(DBloodActor*);
void TommySeqCallback(DBloodActor*);
void TeslaSeqCallback(DBloodActor*);
void ShotSeqCallback(DBloodActor*);
void cultThrowSeqCallback(DBloodActor*);
void cultThrowSeqCallback2(DBloodActor*);
void cultThrowSeqCallback3(DBloodActor*);
void SlashFSeqCallback(DBloodActor*);
void ThrowFSeqCallback(DBloodActor*);
void BlastSSeqCallback(DBloodActor*);
void ThrowSSeqCallback(DBloodActor*);
void ghostSlashSeqCallback(DBloodActor*);
void ghostThrowSeqCallback(DBloodActor*);
void ghostBlastSeqCallback(DBloodActor*);
void GillBiteSeqCallback(DBloodActor*);
void HandJumpSeqCallback(DBloodActor*);
void houndBiteSeqCallback(DBloodActor*);
void houndBurnSeqCallback(DBloodActor*);
void podPlaySound1(DBloodActor*);
void podPlaySound2(DBloodActor*);
void podAttack(DBloodActor*);
void podExplode(DBloodActor*);
void ratBiteSeqCallback(DBloodActor*);
void SpidBiteSeqCallback(DBloodActor*);
void SpidJumpSeqCallback(DBloodActor*);
void SpidBirthSeqCallback(DBloodActor*);
void tchernobogFire(DBloodActor*);
void tchernobogBurnSeqCallback(DBloodActor*);
void tchernobogBurnSeqCallback2(DBloodActor*);
void genDudeAttack1(DBloodActor*);
void punchCallback(DBloodActor*);
void ThrowCallback1(DBloodActor*);
void ThrowCallback2(DBloodActor*);
void HackSeqCallback(DBloodActor*);
void StandSeqCallback(DBloodActor*);
void zombfHackSeqCallback(DBloodActor*);
void PukeSeqCallback(DBloodActor*);
void ThrowSeqCallback(DBloodActor*);
void PlayerSurvive(DBloodActor*);
void PlayerKneelsOver(DBloodActor*);
void FireballTrapSeqCallback(DBloodActor*);
void MGunFireSeqCallback(DBloodActor*);
void MGunOpenSeqCallback(DBloodActor*);

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
	nSpidBirthClient,
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
