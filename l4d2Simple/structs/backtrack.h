#pragma once

struct CTickRecord;

struct CValidTick
{
	explicit operator CTickRecord() const;

	explicit operator bool() const noexcept
	{
		return m_flSimulationTime > 0.f;
	}

	float m_flPitch = 0.f;
	float m_flYaw = 0.f;
	float m_flSimulationTime = 0.f;
	CBaseEntity* m_pEntity = nullptr;
};

struct CTickRecord
{
	CTickRecord() {}
	CTickRecord(CBaseEntity* ent)
	{
		m_flLowerBodyYawTarget = ent->GetNetProp<float>("m_flLowerBodyYawTarget", "DT_BaseAnimating");
		m_angEyeAngles = ent->GetEyeAngles();
		m_flCycle = ent->GetNetProp<float>("m_flCycle", "DT_BaseAnimating");
		m_flSimulationTime = ent->GetNetProp<float>("m_flSimulationTime", "DT_BaseAnimating");
		m_nSequence = ent->GetNetProp<int>("m_nSequence", "DT_BaseAnimating");
		m_vecOrigin = ent->GetNetProp<Vector>("m_vecOrigin", "DT_BaseAnimating");
		m_vecVelocity = ent->GetNetProp<Vector>("m_vecVelocity[0]", "DT_BaseAnimating");
		m_flPoseParameter = ent->GetNetProp<std::array<float, 24>>("m_flPoseParameter", "DT_BaseAnimating");
		m_angAbsAngles = ent->GetAbsAngles();
		m_vecAbsOrigin = ent->GetAbsOrigin();
		tickcount = 0;
	}

	explicit operator bool() const noexcept
	{
		return m_flSimulationTime > 0.f;
	}

	bool operator>(const CTickRecord& others)
	{
		return (m_flSimulationTime > others.m_flSimulationTime);
	}
	bool operator>=(const CTickRecord& others)
	{
		return (m_flSimulationTime >= others.m_flSimulationTime);
	}
	bool operator<(const CTickRecord& others)
	{
		return (m_flSimulationTime < others.m_flSimulationTime);
	}
	bool operator<=(const CTickRecord& others)
	{
		return (m_flSimulationTime <= others.m_flSimulationTime);
	}
	bool operator==(const CTickRecord& others)
	{
		return (m_flSimulationTime == others.m_flSimulationTime);
	}

	float m_flLowerBodyYawTarget = 0.f;
	QAngle m_angEyeAngles = QAngle(0, 0, 0);
	float m_flCycle = 0.f;
	float m_flSimulationTime = 0.f;
	int m_nSequence = 0;
	Vector m_vecOrigin = Vector(0, 0, 0);
	Vector m_vecAbsOrigin = Vector(0, 0, 0);
	Vector m_vecVelocity = Vector(0, 0, 0);
	std::array<float, 24> m_flPoseParameter = {};
	QAngle m_angAbsAngles = QAngle(0, 0, 0);
	CValidTick validtick;
	int tickcount = 0;
};

inline CValidTick::operator CTickRecord() const
{
	CTickRecord rec(m_pEntity);
	rec.m_angEyeAngles.x = this->m_flPitch;
	rec.m_angEyeAngles.y = this->m_flYaw;
	rec.m_flSimulationTime = this->m_flSimulationTime;
	return rec;
}
